/*
 * OHBridge with software rendering — writes frames to a shared framebuffer file.
 * An Android viewer app reads the file and displays it on a SurfaceView.
 *
 * Entry point: JNI_OnLoad_ohbridge (called from Runtime_nativeLoad)
 * Framebuffer: /data/local/tmp/a2oh/framebuffer.raw (ARGB, 480x800)
 * Touch input: /data/local/tmp/a2oh/touch.dat (binary: action,x,y as int32s)
 */
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

/* ── Framebuffer ── */
#define FB_W 480
#define FB_H 800
#define FB_PATH "/data/local/tmp/a2oh/framebuffer.raw"
#define TOUCH_PATH "/data/local/tmp/a2oh/touch.dat"

static uint32_t* g_fb = NULL;  /* Pixel buffer ARGB */
static int g_fb_fd = -1;
static int g_fb_w = FB_W, g_fb_h = FB_H;

static void fb_init(void) {
    if (g_fb) return;
    size_t sz = g_fb_w * g_fb_h * 4;
    g_fb_fd = open(FB_PATH, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (g_fb_fd < 0) {
        g_fb = (uint32_t*)calloc(g_fb_w * g_fb_h, 4);
        return;
    }
    ftruncate(g_fb_fd, sz);
    g_fb = (uint32_t*)mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, g_fb_fd, 0);
    if (g_fb == MAP_FAILED) {
        g_fb = (uint32_t*)calloc(g_fb_w * g_fb_h, 4);
        close(g_fb_fd); g_fb_fd = -1;
    }
}

/* ── Basic 8x8 bitmap font (printable ASCII 32-126) ── */
static const uint8_t font8x8[][8] = {
    {0,0,0,0,0,0,0,0},           /* 32 space */
    {24,60,60,24,24,0,24,0},     /* 33 ! */
    {54,54,0,0,0,0,0,0},         /* 34 " */
    {54,54,127,54,127,54,54,0},  /* 35 # */
    {12,62,3,30,48,31,12,0},     /* 36 $ */
    {0,99,51,24,12,102,99,0},    /* 37 % */
    {28,54,28,110,59,51,110,0},  /* 38 & */
    {6,6,3,0,0,0,0,0},           /* 39 ' */
    {24,12,6,6,6,12,24,0},       /* 40 ( */
    {6,12,24,24,24,12,6,0},      /* 41 ) */
    {0,102,60,255,60,102,0,0},   /* 42 * */
    {0,12,12,63,12,12,0,0},      /* 43 + */
    {0,0,0,0,0,12,12,6},         /* 44 , */
    {0,0,0,63,0,0,0,0},          /* 45 - */
    {0,0,0,0,0,12,12,0},         /* 46 . */
    {96,48,24,12,6,3,1,0},       /* 47 / */
    {62,99,115,123,111,103,62,0},/* 48 0 */
    {12,14,12,12,12,12,63,0},    /* 49 1 */
    {30,51,48,24,12,6,63,0},     /* 50 2 */
    {30,51,48,28,48,51,30,0},    /* 51 3 */
    {56,60,54,51,127,48,120,0},  /* 52 4 */
    {63,3,31,48,48,51,30,0},     /* 53 5 */
    {28,6,3,31,51,51,30,0},      /* 54 6 */
    {63,51,48,24,12,12,12,0},    /* 55 7 */
    {30,51,51,30,51,51,30,0},    /* 56 8 */
    {30,51,51,62,48,24,14,0},    /* 57 9 */
    {0,12,12,0,0,12,12,0},       /* 58 : */
    {0,12,12,0,0,12,12,6},       /* 59 ; */
    {24,12,6,3,6,12,24,0},       /* 60 < */
    {0,0,63,0,0,63,0,0},         /* 61 = */
    {6,12,24,48,24,12,6,0},      /* 62 > */
    {30,51,48,24,12,0,12,0},     /* 63 ? */
    {62,99,123,123,123,3,30,0},  /* 64 @ */
    {12,30,51,51,63,51,51,0},    /* 65 A */
    {63,102,102,62,102,102,63,0},/* 66 B */
    {60,102,3,3,3,102,60,0},     /* 67 C */
    {31,54,102,102,102,54,31,0}, /* 68 D */
    {127,70,22,30,22,70,127,0},  /* 69 E */
    {127,70,22,30,22,6,15,0},    /* 70 F */
    {60,102,3,3,115,102,92,0},   /* 71 G */
    {51,51,51,63,51,51,51,0},    /* 72 H */
    {30,12,12,12,12,12,30,0},    /* 73 I */
    {120,48,48,48,51,51,30,0},   /* 74 J */
    {103,102,54,30,54,102,103,0},/* 75 K */
    {15,6,6,6,70,102,127,0},     /* 76 L */
    {99,119,127,127,107,99,99,0},/* 77 M */
    {99,103,111,123,115,99,99,0},/* 78 N */
    {28,54,99,99,99,54,28,0},    /* 79 O */
    {63,102,102,62,6,6,15,0},    /* 80 P */
    {30,51,51,51,59,30,56,0},    /* 81 Q */
    {63,102,102,62,54,102,103,0},/* 82 R */
    {30,51,7,14,56,51,30,0},     /* 83 S */
    {63,45,12,12,12,12,30,0},    /* 84 T */
    {51,51,51,51,51,51,63,0},    /* 85 U */
    {51,51,51,51,51,30,12,0},    /* 86 V */
    {99,99,99,107,127,119,99,0}, /* 87 W */
    {99,99,54,28,28,54,99,0},    /* 88 X */
    {51,51,51,30,12,12,30,0},    /* 89 Y */
    {127,99,49,24,12,70,127,0},  /* 90 Z */
    {30,6,6,6,6,6,30,0},         /* 91 [ */
    {1,3,6,12,24,48,96,0},       /* 92 \ */
    {30,24,24,24,24,24,30,0},    /* 93 ] */
    {8,28,54,99,0,0,0,0},        /* 94 ^ */
    {0,0,0,0,0,0,0,255},         /* 95 _ */
    {12,12,24,0,0,0,0,0},        /* 96 ` */
    {0,0,30,48,62,51,110,0},     /* 97 a */
    {7,6,6,62,102,102,59,0},     /* 98 b */
    {0,0,30,51,3,51,30,0},       /* 99 c */
    {56,48,48,62,51,51,110,0},   /* 100 d */
    {0,0,30,51,63,3,30,0},       /* 101 e */
    {28,54,6,15,6,6,15,0},       /* 102 f */
    {0,0,110,51,51,62,48,31},    /* 103 g */
    {7,6,54,110,102,102,103,0},  /* 104 h */
    {12,0,14,12,12,12,30,0},     /* 105 i */
    {48,0,48,48,48,51,51,30},    /* 106 j */
    {7,6,102,54,30,54,103,0},    /* 107 k */
    {14,12,12,12,12,12,30,0},    /* 108 l */
    {0,0,51,127,127,107,99,0},   /* 109 m */
    {0,0,31,51,51,51,51,0},      /* 110 n */
    {0,0,30,51,51,51,30,0},      /* 111 o */
    {0,0,59,102,102,62,6,15},    /* 112 p */
    {0,0,110,51,51,62,48,120},   /* 113 q */
    {0,0,59,110,102,6,15,0},     /* 114 r */
    {0,0,62,3,30,48,31,0},       /* 115 s */
    {8,12,62,12,12,44,24,0},     /* 116 t */
    {0,0,51,51,51,51,110,0},     /* 117 u */
    {0,0,51,51,51,30,12,0},      /* 118 v */
    {0,0,99,107,127,54,54,0},    /* 119 w */
    {0,0,99,54,28,54,99,0},      /* 120 x */
    {0,0,51,51,51,62,48,31},     /* 121 y */
    {0,0,63,25,12,38,63,0},      /* 122 z */
    {56,12,12,7,12,12,56,0},     /* 123 { */
    {24,24,24,0,24,24,24,0},     /* 124 | */
    {7,12,12,56,12,12,7,0},      /* 125 } */
    {110,59,0,0,0,0,0,0},        /* 126 ~ */
};

/* ── Drawing primitives ── */

static inline void px_set(int x, int y, uint32_t color) {
    if (x >= 0 && x < g_fb_w && y >= 0 && y < g_fb_h)
        g_fb[y * g_fb_w + x] = color;
}

static void draw_rect_fill(int x1, int y1, int x2, int y2, uint32_t color) {
    if (x1 > x2) { int t=x1; x1=x2; x2=t; }
    if (y1 > y2) { int t=y1; y1=y2; y2=t; }
    if (x1 < 0) x1 = 0; if (y1 < 0) y1 = 0;
    if (x2 >= g_fb_w) x2 = g_fb_w-1; if (y2 >= g_fb_h) y2 = g_fb_h-1;
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            g_fb[y * g_fb_w + x] = color;
}

static void draw_text(const char* text, int x, int y, uint32_t color, int scale) {
    if (!text || scale < 1) return;
    int ox = x;
    for (; *text; text++) {
        if (*text == '\n') { y += 8*scale; x = ox; continue; }
        int ch = *text;
        if (ch < 32 || ch > 126) ch = '?';
        const uint8_t* glyph = font8x8[ch - 32];
        for (int gy = 0; gy < 8; gy++)
            for (int gx = 0; gx < 8; gx++)
                if (glyph[gy] & (1 << gx))
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++)
                            px_set(x + gx*scale + sx, y + gy*scale + sy, color);
        x += 8*scale;
    }
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    for (;;) {
        px_set(x0, y0, color);
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_circle(int cx, int cy, int r, uint32_t color, int fill) {
    int x = r, y = 0, err = 1-r;
    while (x >= y) {
        if (fill) {
            draw_rect_fill(cx-x, cy+y, cx+x, cy+y, color);
            draw_rect_fill(cx-x, cy-y, cx+x, cy-y, color);
            draw_rect_fill(cx-y, cy+x, cx+y, cy+x, color);
            draw_rect_fill(cx-y, cy-x, cx+y, cy-x, color);
        } else {
            px_set(cx+x,cy+y,color); px_set(cx-x,cy+y,color);
            px_set(cx+x,cy-y,color); px_set(cx-x,cy-y,color);
            px_set(cx+y,cy+x,color); px_set(cx-y,cy+x,color);
            px_set(cx+y,cy-x,color); px_set(cx-y,cy-x,color);
        }
        y++;
        if (err < 0) err += 2*y+1;
        else { x--; err += 2*(y-x)+1; }
    }
}

static void draw_round_rect(int x1, int y1, int x2, int y2, int rx, int ry, uint32_t color) {
    if (rx > (x2-x1)/2) rx = (x2-x1)/2;
    if (ry > (y2-y1)/2) ry = (y2-y1)/2;
    draw_rect_fill(x1+rx, y1, x2-rx, y2, color);
    draw_rect_fill(x1, y1+ry, x1+rx, y2-ry, color);
    draw_rect_fill(x2-rx, y1+ry, x2, y2-ry, color);
    /* Corner circles simplified as filled rects for now */
    draw_rect_fill(x1, y1, x1+rx, y1+ry, color);
    draw_rect_fill(x2-rx, y1, x2, y1+ry, color);
    draw_rect_fill(x1, y2-ry, x1+rx, y2, color);
    draw_rect_fill(x2-rx, y2-ry, x2, y2, color);
}

/* ── Canvas state (simple stack) ── */
typedef struct {
    int clip_x1, clip_y1, clip_x2, clip_y2;
    float tx, ty;  /* translate */
} CanvasState;

#define MAX_SAVE 32
static CanvasState g_states[MAX_SAVE];
static int g_state_top = 0;
static float g_tx = 0, g_ty = 0;

/* ── Pen/Brush state ── */
typedef struct { uint32_t color; float width; int style; /* 0=fill,1=stroke */ } PenBrush;
static PenBrush g_pens[64];
static int g_pen_next = 1;
static PenBrush g_brushes[64];
static int g_brush_next = 1;

/* ── Font state ── */
typedef struct { float size; } FontState;
static FontState g_fonts[32];
static int g_font_next = 1;

/* ── JNI Methods ── */

/* Logging */
static void OHB_logDebug(JNIEnv*e,jclass c,jstring t,jstring m){
    const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
    fprintf(stderr,"[D] %s: %s\n",a,b);
    (*e)->ReleaseStringUTFChars(e,m,b);(*e)->ReleaseStringUTFChars(e,t,a);
}
static void OHB_logInfo(JNIEnv*e,jclass c,jstring t,jstring m){
    const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
    fprintf(stderr,"[I] %s: %s\n",a,b);
    (*e)->ReleaseStringUTFChars(e,m,b);(*e)->ReleaseStringUTFChars(e,t,a);
}
static void OHB_logWarn(JNIEnv*e,jclass c,jstring t,jstring m){
    const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
    fprintf(stderr,"[W] %s: %s\n",a,b);
    (*e)->ReleaseStringUTFChars(e,m,b);(*e)->ReleaseStringUTFChars(e,t,a);
}
static void OHB_logError(JNIEnv*e,jclass c,jstring t,jstring m){
    const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
    fprintf(stderr,"[E] %s: %s\n",a,b);
    (*e)->ReleaseStringUTFChars(e,m,b);(*e)->ReleaseStringUTFChars(e,t,a);
}

/* Canvas */
static jint OHB_arkuiInit(JNIEnv*e,jclass c) { fb_init(); return 0; }

static jlong OHB_surfaceCreate(JNIEnv*e,jclass c,jlong unused,jint w,jint h) {
    g_fb_w = w; g_fb_h = h; fb_init(); return 1;
}
static jlong OHB_surfaceGetCanvas(JNIEnv*e,jclass c,jlong s) { return 1; }
static jint OHB_surfaceFlush(JNIEnv*e,jclass c,jlong s) {
    if (g_fb && g_fb_fd >= 0) msync(g_fb, g_fb_w*g_fb_h*4, MS_SYNC);
    return 0;
}
static void OHB_surfaceDestroy(JNIEnv*e,jclass c,jlong s) {}
static void OHB_surfaceResize(JNIEnv*e,jclass c,jlong s,jint w,jint h) {}

static jlong OHB_canvasCreate(JNIEnv*e,jclass c,jlong bmp) { return 1; }
static void OHB_canvasDestroy(JNIEnv*e,jclass c,jlong cn) {}

static void OHB_canvasDrawColor(JNIEnv*e,jclass c,jlong cn,jint color) {
    if (!g_fb) return;
    uint32_t col = (uint32_t)color;
    for (int i = 0; i < g_fb_w * g_fb_h; i++) g_fb[i] = col;
}

static void OHB_canvasDrawRect(JNIEnv*e,jclass c,jlong cn,jfloat l,jfloat t,jfloat r,jfloat b,jlong pen,jlong brush) {
    if (!g_fb) return;
    uint32_t col = 0xFF000000;
    if (brush > 0 && brush < 64) col = g_brushes[brush].color;
    else if (pen > 0 && pen < 64) col = g_pens[pen].color;
    draw_rect_fill((int)(l+g_tx),(int)(t+g_ty),(int)(r+g_tx),(int)(b+g_ty), col);
}

static void OHB_canvasDrawRoundRect(JNIEnv*e,jclass c,jlong cn,jfloat l,jfloat t,jfloat r,jfloat b,jfloat rx,jfloat ry,jlong pen,jlong brush) {
    if (!g_fb) return;
    uint32_t col = 0xFF000000;
    if (brush > 0 && brush < 64) col = g_brushes[brush].color;
    else if (pen > 0 && pen < 64) col = g_pens[pen].color;
    draw_round_rect((int)(l+g_tx),(int)(t+g_ty),(int)(r+g_tx),(int)(b+g_ty),(int)rx,(int)ry, col);
}

static void OHB_canvasDrawCircle(JNIEnv*e,jclass c,jlong cn,jfloat cx,jfloat cy,jfloat r,jlong pen,jlong brush) {
    if (!g_fb) return;
    uint32_t col = 0xFF000000;
    if (brush > 0 && brush < 64) col = g_brushes[brush].color;
    draw_circle((int)(cx+g_tx),(int)(cy+g_ty),(int)r, col, 1);
}

static void OHB_canvasDrawLine(JNIEnv*e,jclass c,jlong cn,jfloat x1,jfloat y1,jfloat x2,jfloat y2,jlong pen) {
    if (!g_fb) return;
    uint32_t col = 0xFF000000;
    if (pen > 0 && pen < 64) col = g_pens[pen].color;
    draw_line((int)(x1+g_tx),(int)(y1+g_ty),(int)(x2+g_tx),(int)(y2+g_ty), col);
}

static void OHB_canvasDrawText(JNIEnv*e,jclass c,jlong cn,jstring js,jfloat x,jfloat y,jlong font,jlong pen,jlong brush) {
    if (!g_fb || !js) return;
    const char* s = (*e)->GetStringUTFChars(e, js, 0);
    uint32_t col = 0xFF000000;
    if (pen > 0 && pen < 64) col = g_pens[pen].color;
    else if (brush > 0 && brush < 64) col = g_brushes[brush].color;
    int scale = 2;
    if (font > 0 && font < 32) {
        float sz = g_fonts[font].size;
        scale = (int)(sz / 8); if (scale < 1) scale = 1; if (scale > 4) scale = 4;
    }
    draw_text(s, (int)(x+g_tx), (int)(y+g_ty - 8*scale), col, scale);
    (*e)->ReleaseStringUTFChars(e, js, s);
}

static void OHB_canvasSave(JNIEnv*e,jclass c,jlong cn) {
    if (g_state_top < MAX_SAVE) {
        g_states[g_state_top].tx = g_tx;
        g_states[g_state_top].ty = g_ty;
        g_state_top++;
    }
}
static void OHB_canvasRestore(JNIEnv*e,jclass c,jlong cn) {
    if (g_state_top > 0) {
        g_state_top--;
        g_tx = g_states[g_state_top].tx;
        g_ty = g_states[g_state_top].ty;
    }
}
static void OHB_canvasTranslate(JNIEnv*e,jclass c,jlong cn,jfloat dx,jfloat dy) { g_tx+=dx; g_ty+=dy; }
static void OHB_canvasScale(JNIEnv*e,jclass c,jlong cn,jfloat sx,jfloat sy) {}
static void OHB_canvasRotate(JNIEnv*e,jclass c,jlong cn,jfloat deg,jfloat px,jfloat py) {}
static void OHB_canvasClipRect(JNIEnv*e,jclass c,jlong cn,jfloat l,jfloat t,jfloat r,jfloat b) {}
static void OHB_canvasClipPath(JNIEnv*e,jclass c,jlong cn,jlong p) {}
static void OHB_canvasConcat(JNIEnv*e,jclass c,jlong cn,jfloatArray m) {}
static void OHB_canvasDrawOval(JNIEnv*e,jclass c,jlong cn,jfloat l,jfloat t,jfloat r,jfloat b,jlong pen,jlong brush) {}
static void OHB_canvasDrawArc(JNIEnv*e,jclass c,jlong cn,jfloat l,jfloat t,jfloat r,jfloat b,jfloat sa,jfloat sw,jboolean uc,jlong pen,jlong brush) {}
static void OHB_canvasDrawPath(JNIEnv*e,jclass c,jlong cn,jlong p,jlong pen,jlong brush) {}
static void OHB_canvasDrawBitmap(JNIEnv*e,jclass c,jlong cn,jlong bmp,jfloat x,jfloat y) {}
static void OHB_canvasDrawBitmapNine(JNIEnv*e,jclass c,jlong cn,jlong bmp,jint cl2,jint ct,jint cr,jint cb,jfloat dl,jfloat dt,jfloat dr,jfloat db,jlong p) {}

/* Pen/Brush */
static jlong OHB_penCreate(JNIEnv*e,jclass c) { int id=g_pen_next++; if(id>=64)id=1; g_pens[id].color=0xFF000000; g_pens[id].width=1; return id; }
static void OHB_penSetColor(JNIEnv*e,jclass c,jlong p,jint col) { if(p>0&&p<64) g_pens[p].color=(uint32_t)col; }
static void OHB_penSetWidth(JNIEnv*e,jclass c,jlong p,jfloat w) { if(p>0&&p<64) g_pens[p].width=w; }
static void OHB_penSetAntiAlias(JNIEnv*e,jclass c,jlong p,jboolean aa) {}
static void OHB_penSetCap(JNIEnv*e,jclass c,jlong p,jint cap) {}
static void OHB_penSetJoin(JNIEnv*e,jclass c,jlong p,jint j) {}
static void OHB_penDestroy(JNIEnv*e,jclass c,jlong p) {}

static jlong OHB_brushCreate(JNIEnv*e,jclass c) { int id=g_brush_next++; if(id>=64)id=1; g_brushes[id].color=0xFF000000; return id; }
static void OHB_brushSetColor(JNIEnv*e,jclass c,jlong b,jint col) { if(b>0&&b<64) g_brushes[b].color=(uint32_t)col; }
static void OHB_brushSetAntiAlias(JNIEnv*e,jclass c,jlong b,jboolean aa) {}
static void OHB_brushDestroy(JNIEnv*e,jclass c,jlong b) {}

/* Font */
static jlong OHB_fontCreate(JNIEnv*e,jclass c,jfloat sz) { int id=g_font_next++; if(id>=32)id=1; g_fonts[id].size=sz; return id; }
static void OHB_fontSetSize(JNIEnv*e,jclass c,jlong f,jfloat sz) { if(f>0&&f<32) g_fonts[f].size=sz; }
static jfloat OHB_fontMeasureText(JNIEnv*e,jclass c,jlong f,jstring s) {
    if(!s) return 0;
    int len = (*e)->GetStringLength(e, s);
    float sz = (f>0&&f<32) ? g_fonts[f].size : 16;
    int scale = (int)(sz/8); if(scale<1) scale=1;
    return (jfloat)(len * 8 * scale);
}
static void OHB_fontDestroy(JNIEnv*e,jclass c,jlong f) {}
static jfloatArray OHB_fontGetMetrics(JNIEnv*e,jclass c,jlong f) {
    jfloatArray arr=(*e)->NewFloatArray(e,4);
    float sz = (f>0&&f<32) ? g_fonts[f].size : 16;
    jfloat m[]={-sz*0.8f, sz*0.2f, 0, sz};
    (*e)->SetFloatArrayRegion(e,arr,0,4,m); return arr;
}

/* Bitmap */
static jlong OHB_bitmapCreate(JNIEnv*e,jclass c,jint w,jint h,jint fmt) { return 1; }
static void OHB_bitmapDestroy(JNIEnv*e,jclass c,jlong b) {}
static jint OHB_bitmapGetWidth(JNIEnv*e,jclass c,jlong b) { return g_fb_w; }
static jint OHB_bitmapGetHeight(JNIEnv*e,jclass c,jlong b) { return g_fb_h; }
static void OHB_bitmapSetPixel(JNIEnv*e,jclass c,jlong b,jint x,jint y,jint col) { px_set(x,y,(uint32_t)col); }
static jint OHB_bitmapGetPixel(JNIEnv*e,jclass c,jlong b,jint x,jint y) { return 0; }
static jint OHB_bitmapWriteToFile(JNIEnv*e,jclass c,jlong b,jstring path) { return 0; }
static jint OHB_bitmapBlitToFb0(JNIEnv*e,jclass c,jlong b,jint off) { return 0; }

/* Preferences (in-memory only) */
static jlong OHB_preferencesOpen(JNIEnv*e,jclass c,jstring n) { return 1; }
static jstring OHB_preferencesGetString(JNIEnv*e,jclass c,jlong h,jstring k,jstring d) { return d; }
static jint OHB_preferencesGetInt(JNIEnv*e,jclass c,jlong h,jstring k,jint d) { return d; }
static jlong OHB_preferencesGetLong(JNIEnv*e,jclass c,jlong h,jstring k,jlong d) { return d; }
static jfloat OHB_preferencesGetFloat(JNIEnv*e,jclass c,jlong h,jstring k,jfloat d) { return d; }
static jboolean OHB_preferencesGetBoolean(JNIEnv*e,jclass c,jlong h,jstring k,jboolean d) { return d; }
static void OHB_preferencesPutString(JNIEnv*e,jclass c,jlong h,jstring k,jstring v) {}
static void OHB_preferencesPutInt(JNIEnv*e,jclass c,jlong h,jstring k,jint v) {}
static void OHB_preferencesPutLong(JNIEnv*e,jclass c,jlong h,jstring k,jlong v) {}
static void OHB_preferencesPutFloat(JNIEnv*e,jclass c,jlong h,jstring k,jfloat v) {}
static void OHB_preferencesPutBoolean(JNIEnv*e,jclass c,jlong h,jstring k,jboolean v) {}
static void OHB_preferencesFlush(JNIEnv*e,jclass c,jlong h) {}
static void OHB_preferencesRemove(JNIEnv*e,jclass c,jlong h,jstring k) {}
static void OHB_preferencesClear(JNIEnv*e,jclass c,jlong h) {}
static void OHB_preferencesClose(JNIEnv*e,jclass c,jlong h) {}

/* RDB stubs */
static jlong OHB_rdbStoreOpen(JNIEnv*e,jclass c,jstring n,jint v) { return 1; }
static void OHB_rdbStoreExecSQL(JNIEnv*e,jclass c,jlong h,jstring sql) {}
static jlong OHB_rdbStoreQuery(JNIEnv*e,jclass c,jlong h,jstring sql,jobjectArray a) { return 0; }
static jlong OHB_rdbStoreInsert(JNIEnv*e,jclass c,jlong h,jstring t,jstring j) { return 1; }
static jint OHB_rdbStoreUpdate(JNIEnv*e,jclass c,jlong h,jstring j,jstring t,jstring w,jobjectArray a) { return 0; }
static jint OHB_rdbStoreDelete(JNIEnv*e,jclass c,jlong h,jstring t,jstring w,jobjectArray a) { return 0; }
static void OHB_rdbStoreBeginTransaction(JNIEnv*e,jclass c,jlong h) {}
static void OHB_rdbStoreCommit(JNIEnv*e,jclass c,jlong h) {}
static void OHB_rdbStoreRollback(JNIEnv*e,jclass c,jlong h) {}
static void OHB_rdbStoreClose(JNIEnv*e,jclass c,jlong h) {}
static jboolean OHB_resultSetGoToFirstRow(JNIEnv*e,jclass c,jlong h) { return JNI_FALSE; }
static jboolean OHB_resultSetGoToNextRow(JNIEnv*e,jclass c,jlong h) { return JNI_FALSE; }
static jint OHB_resultSetGetColumnIndex(JNIEnv*e,jclass c,jlong h,jstring n) { return -1; }
static jstring OHB_resultSetGetString(JNIEnv*e,jclass c,jlong h,jint i) { return (*e)->NewStringUTF(e,""); }
static jint OHB_resultSetGetInt(JNIEnv*e,jclass c,jlong h,jint i) { return 0; }
static jlong OHB_resultSetGetLong(JNIEnv*e,jclass c,jlong h,jint i) { return 0; }
static jfloat OHB_resultSetGetFloat(JNIEnv*e,jclass c,jlong h,jint i) { return 0; }
static jdouble OHB_resultSetGetDouble(JNIEnv*e,jclass c,jlong h,jint i) { return 0; }
static jbyteArray OHB_resultSetGetBlob(JNIEnv*e,jclass c,jlong h,jint i) { return NULL; }
static jint OHB_resultSetGetColumnCount(JNIEnv*e,jclass c,jlong h) { return 0; }
static jstring OHB_resultSetGetColumnName(JNIEnv*e,jclass c,jlong h,jint i) { return (*e)->NewStringUTF(e,""); }
static jint OHB_resultSetGetRowCount(JNIEnv*e,jclass c,jlong h) { return 0; }
static jboolean OHB_resultSetIsNull(JNIEnv*e,jclass c,jlong h,jint i) { return JNI_TRUE; }
static void OHB_resultSetClose(JNIEnv*e,jclass c,jlong h) {}

/* Path stubs */
static jlong OHB_pathCreate(JNIEnv*e,jclass c) { return 1; }
static void OHB_pathMoveTo(JNIEnv*e,jclass c,jlong p,jfloat x,jfloat y) {}
static void OHB_pathLineTo(JNIEnv*e,jclass c,jlong p,jfloat x,jfloat y) {}
static void OHB_pathClose(JNIEnv*e,jclass c,jlong p) {}
static void OHB_pathDestroy(JNIEnv*e,jclass c,jlong p) {}
static void OHB_pathCubicTo(JNIEnv*e,jclass c,jlong p,jfloat x1,jfloat y1,jfloat x2,jfloat y2,jfloat x3,jfloat y3) {}
static void OHB_pathQuadTo(JNIEnv*e,jclass c,jlong p,jfloat x1,jfloat y1,jfloat x2,jfloat y2) {}
static void OHB_pathAddRect(JNIEnv*e,jclass c,jlong p,jfloat l,jfloat t,jfloat r,jfloat b,jint d) {}
static void OHB_pathAddCircle(JNIEnv*e,jclass c,jlong p,jfloat cx,jfloat cy,jfloat r,jint d) {}
static void OHB_pathReset(JNIEnv*e,jclass c,jlong p) {}

/* Misc stubs — all return 0/null/false */
static void OHB_showToast(JNIEnv*e,jclass c,jstring m,jint d) {}
static jint OHB_checkPermission(JNIEnv*e,jclass c,jstring p) { return 0; }
static jstring OHB_getDeviceBrand(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,"Westlake"); }
static jstring OHB_getDeviceModel(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,"Phone"); }
static jstring OHB_getOSVersion(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,"1.0"); }
static jint OHB_getNetworkType(JNIEnv*e,jclass c) { return 0; }
static void OHB_startAbility(JNIEnv*e,jclass c,jstring a,jstring b,jstring d) {}
static void OHB_terminateSelf(JNIEnv*e,jclass c) {}
static jstring OHB_httpRequest(JNIEnv*e,jclass c,jstring u,jstring m,jstring h,jstring b) { return NULL; }
static void OHB_clipboardSet(JNIEnv*e,jclass c,jstring t) {}
static jstring OHB_clipboardGet(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,""); }
static jboolean OHB_vibratorHasVibrator(JNIEnv*e,jclass c) { return JNI_FALSE; }
static void OHB_vibratorVibrate(JNIEnv*e,jclass c,jlong ms) {}
static void OHB_vibratorCancel(JNIEnv*e,jclass c) {}
static jboolean OHB_locationIsEnabled(JNIEnv*e,jclass c) { return JNI_FALSE; }
static jdoubleArray OHB_locationGetLast(JNIEnv*e,jclass c) { return NULL; }
static jboolean OHB_sensorIsAvailable(JNIEnv*e,jclass c,jint t) { return JNI_FALSE; }
static jfloatArray OHB_sensorGetData(JNIEnv*e,jclass c,jint t) { return NULL; }
static jlong OHB_mediaPlayerCreate(JNIEnv*e,jclass c) { return 0; }
static void OHB_mediaPlayerSetDataSource(JNIEnv*e,jclass c,jlong h,jstring p) {}
static void OHB_mediaPlayerPrepare(JNIEnv*e,jclass c,jlong h) {}
static void OHB_mediaPlayerStart(JNIEnv*e,jclass c,jlong h) {}
static void OHB_mediaPlayerPause(JNIEnv*e,jclass c,jlong h) {}
static void OHB_mediaPlayerStop(JNIEnv*e,jclass c,jlong h) {}
static void OHB_mediaPlayerRelease(JNIEnv*e,jclass c,jlong h) {}
static void OHB_mediaPlayerReset(JNIEnv*e,jclass c,jlong h) {}
static void OHB_mediaPlayerSeekTo(JNIEnv*e,jclass c,jlong h,jint ms) {}
static jint OHB_mediaPlayerGetCurrentPosition(JNIEnv*e,jclass c,jlong h) { return 0; }
static jint OHB_mediaPlayerGetDuration(JNIEnv*e,jclass c,jlong h) { return 0; }
static jboolean OHB_mediaPlayerIsPlaying(JNIEnv*e,jclass c,jlong h) { return JNI_FALSE; }
static void OHB_mediaPlayerSetLooping(JNIEnv*e,jclass c,jlong h,jboolean l) {}
static void OHB_mediaPlayerSetVolume(JNIEnv*e,jclass c,jlong h,jfloat lv,jfloat rv) {}
static jint OHB_audioGetStreamVolume(JNIEnv*e,jclass c,jint s) { return 50; }
static jint OHB_audioGetStreamMaxVolume(JNIEnv*e,jclass c,jint s) { return 100; }
static void OHB_audioSetStreamVolume(JNIEnv*e,jclass c,jint s,jint v,jint f) {}
static jint OHB_audioGetRingerMode(JNIEnv*e,jclass c) { return 2; }
static void OHB_audioSetRingerMode(JNIEnv*e,jclass c,jint m) {}
static jboolean OHB_audioIsMusicActive(JNIEnv*e,jclass c) { return JNI_FALSE; }
static jint OHB_telephonyGetPhoneType(JNIEnv*e,jclass c) { return 0; }
static jint OHB_telephonyGetNetworkType(JNIEnv*e,jclass c) { return 0; }
static jint OHB_telephonyGetSimState(JNIEnv*e,jclass c) { return 0; }
static jstring OHB_telephonyGetDeviceId(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,""); }
static jstring OHB_telephonyGetLine1Number(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,""); }
static jstring OHB_telephonyGetNetworkOperatorName(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,""); }
static jboolean OHB_wifiIsEnabled(JNIEnv*e,jclass c) { return JNI_FALSE; }
static jboolean OHB_wifiSetEnabled(JNIEnv*e,jclass c,jboolean en) { return JNI_FALSE; }
static jint OHB_wifiGetState(JNIEnv*e,jclass c) { return 0; }
static jstring OHB_wifiGetSSID(JNIEnv*e,jclass c) { return (*e)->NewStringUTF(e,""); }
static jint OHB_wifiGetRssi(JNIEnv*e,jclass c) { return -50; }
static jint OHB_wifiGetLinkSpeed(JNIEnv*e,jclass c) { return 0; }
static jint OHB_wifiGetFrequency(JNIEnv*e,jclass c) { return 0; }
static void OHB_notificationPublish(JNIEnv*e,jclass c,jint id,jstring ch,jstring t,jstring tx,jint i) {}
static void OHB_notificationCancel(JNIEnv*e,jclass c,jint id) {}
static void OHB_notificationAddSlot(JNIEnv*e,jclass c,jstring id,jstring n,jint imp) {}
static jint OHB_reminderScheduleTimer(JNIEnv*e,jclass c,jint id,jstring t,jstring tx,jstring ch,jstring d) { return 0; }
static void OHB_reminderCancel(JNIEnv*e,jclass c,jint id) {}
static jlong OHB_nodeCreate(JNIEnv*e,jclass c,jint t) { return 0; }
static void OHB_nodeDispose(JNIEnv*e,jclass c,jlong n) {}
static void OHB_nodeAddChild(JNIEnv*e,jclass c,jlong p,jlong ch) {}
static void OHB_nodeRemoveChild(JNIEnv*e,jclass c,jlong p,jlong ch) {}
static void OHB_nodeInsertChildAt(JNIEnv*e,jclass c,jlong p,jlong ch,jint i) {}
static jint OHB_nodeSetAttrInt(JNIEnv*e,jclass c,jlong n,jint a,jint v) { return 0; }
static jint OHB_nodeSetAttrFloat(JNIEnv*e,jclass c,jlong n,jint a,jfloat v1,jfloat v2,jfloat v3,jfloat v4,jint u) { return 0; }
static jint OHB_nodeSetAttrColor(JNIEnv*e,jclass c,jlong n,jint a,jint col) { return 0; }
static jint OHB_nodeSetAttrString(JNIEnv*e,jclass c,jlong n,jint a,jstring v) { return 0; }
static void OHB_nodeMarkDirty(JNIEnv*e,jclass c,jlong n,jint f) {}
static jint OHB_nodeRegisterEvent(JNIEnv*e,jclass c,jlong n,jint ev,jint id) { return 0; }
static void OHB_nodeUnregisterEvent(JNIEnv*e,jclass c,jlong n,jint ev) {}
static void OHB_dispatchKeyEvent(JNIEnv*e,jclass c,jint action,jint keyCode,jlong downTime) {}
static jobject OHB_getResumedActivity(JNIEnv*e,jclass c) { return NULL; }
JNIEXPORT jint JNICALL JNI_OnLoad_ohbridge(JavaVM*vm,void*r){
  fprintf(stderr, "[OHBridge] JNI_OnLoad_ohbridge CALLED\n");
  JNIEnv*e; if((*vm)->GetEnv(vm,(void**)&e,JNI_VERSION_1_6)!=JNI_OK)return-1;
  jclass c=(*e)->FindClass(e,"com/ohos/shim/bridge/OHBridge");
  if(!c){fprintf(stderr, "[OHBridge ARM64] class not found\n");return JNI_VERSION_1_6;}
  JNINativeMethod m[]={
    {"arkuiInit","()I",(void*)OHB_arkuiInit},
    {"audioGetRingerMode","()I",(void*)OHB_audioGetRingerMode},
    {"audioGetStreamMaxVolume","(I)I",(void*)OHB_audioGetStreamMaxVolume},
    {"audioGetStreamVolume","(I)I",(void*)OHB_audioGetStreamVolume},
    {"audioIsMusicActive","()Z",(void*)OHB_audioIsMusicActive},
    {"audioSetRingerMode","(I)V",(void*)OHB_audioSetRingerMode},
    {"audioSetStreamVolume","(III)V",(void*)OHB_audioSetStreamVolume},
    {"bitmapBlitToFb0","(JI)I",(void*)OHB_bitmapBlitToFb0},
    {"bitmapCreate","(III)J",(void*)OHB_bitmapCreate},
    {"bitmapDestroy","(J)V",(void*)OHB_bitmapDestroy},
    {"bitmapGetHeight","(J)I",(void*)OHB_bitmapGetHeight},
    {"bitmapGetPixel","(JII)I",(void*)OHB_bitmapGetPixel},
    {"bitmapGetWidth","(J)I",(void*)OHB_bitmapGetWidth},
    {"bitmapSetPixel","(JIII)V",(void*)OHB_bitmapSetPixel},
    {"bitmapWriteToFile","(JLjava/lang/String;)I",(void*)OHB_bitmapWriteToFile},
    {"brushCreate","()J",(void*)OHB_brushCreate},
    {"brushDestroy","(J)V",(void*)OHB_brushDestroy},
    {"brushSetColor","(JI)V",(void*)OHB_brushSetColor},
    {"canvasClipPath","(JJ)V",(void*)OHB_canvasClipPath},
    {"canvasClipRect","(JFFFF)V",(void*)OHB_canvasClipRect},
    {"canvasConcat","(J[F)V",(void*)OHB_canvasConcat},
    {"canvasCreate","(J)J",(void*)OHB_canvasCreate},
    {"canvasDestroy","(J)V",(void*)OHB_canvasDestroy},
    {"canvasDrawArc","(JFFFFFFZJJ)V",(void*)OHB_canvasDrawArc},
    {"canvasDrawBitmap","(JJFF)V",(void*)OHB_canvasDrawBitmap},
    {"canvasDrawCircle","(JFFFJJ)V",(void*)OHB_canvasDrawCircle},
    {"canvasDrawColor","(JI)V",(void*)OHB_canvasDrawColor},
    {"canvasDrawLine","(JFFFFJ)V",(void*)OHB_canvasDrawLine},
    {"canvasDrawOval","(JFFFFJJ)V",(void*)OHB_canvasDrawOval},
    {"canvasDrawPath","(JJJJ)V",(void*)OHB_canvasDrawPath},
    {"canvasDrawRect","(JFFFFJJ)V",(void*)OHB_canvasDrawRect},
    {"canvasDrawRoundRect","(JFFFFFFJJ)V",(void*)OHB_canvasDrawRoundRect},
    {"canvasDrawText","(JLjava/lang/String;FFJJJ)V",(void*)OHB_canvasDrawText},
    {"canvasRestore","(J)V",(void*)OHB_canvasRestore},
    {"canvasRotate","(JFFF)V",(void*)OHB_canvasRotate},
    {"canvasSave","(J)V",(void*)OHB_canvasSave},
    {"canvasScale","(JFF)V",(void*)OHB_canvasScale},
    {"canvasTranslate","(JFF)V",(void*)OHB_canvasTranslate},
    {"checkPermission","(Ljava/lang/String;)I",(void*)OHB_checkPermission},
    {"clipboardGet","()Ljava/lang/String;",(void*)OHB_clipboardGet},
    {"clipboardSet","(Ljava/lang/String;)V",(void*)OHB_clipboardSet},
    {"fontDestroy","(J)V",(void*)OHB_fontDestroy},
    {"fontGetMetrics","(J)[F",(void*)OHB_fontGetMetrics},
    {"fontMeasureText","(JLjava/lang/String;)F",(void*)OHB_fontMeasureText},
    {"fontSetSize","(JF)V",(void*)OHB_fontSetSize},
    {"getDeviceBrand","()Ljava/lang/String;",(void*)OHB_getDeviceBrand},
    {"getDeviceModel","()Ljava/lang/String;",(void*)OHB_getDeviceModel},
    {"getNetworkType","()I",(void*)OHB_getNetworkType},
    {"getOSVersion","()Ljava/lang/String;",(void*)OHB_getOSVersion},
    {"httpRequest","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",(void*)OHB_httpRequest},
    {"locationGetLast","()[D",(void*)OHB_locationGetLast},
    {"locationIsEnabled","()Z",(void*)OHB_locationIsEnabled},
    {"logDebug","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logDebug},
    {"logError","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logError},
    {"logInfo","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logInfo},
    {"logWarn","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logWarn},
    {"mediaPlayerCreate","()J",(void*)OHB_mediaPlayerCreate},
    {"mediaPlayerGetCurrentPosition","(J)I",(void*)OHB_mediaPlayerGetCurrentPosition},
    {"mediaPlayerGetDuration","(J)I",(void*)OHB_mediaPlayerGetDuration},
    {"mediaPlayerIsPlaying","(J)Z",(void*)OHB_mediaPlayerIsPlaying},
    {"mediaPlayerPause","(J)V",(void*)OHB_mediaPlayerPause},
    {"mediaPlayerPrepare","(J)V",(void*)OHB_mediaPlayerPrepare},
    {"mediaPlayerRelease","(J)V",(void*)OHB_mediaPlayerRelease},
    {"mediaPlayerReset","(J)V",(void*)OHB_mediaPlayerReset},
    {"mediaPlayerSeekTo","(JI)V",(void*)OHB_mediaPlayerSeekTo},
    {"mediaPlayerSetDataSource","(JLjava/lang/String;)V",(void*)OHB_mediaPlayerSetDataSource},
    {"mediaPlayerSetLooping","(JZ)V",(void*)OHB_mediaPlayerSetLooping},
    {"mediaPlayerSetVolume","(JFF)V",(void*)OHB_mediaPlayerSetVolume},
    {"mediaPlayerStart","(J)V",(void*)OHB_mediaPlayerStart},
    {"mediaPlayerStop","(J)V",(void*)OHB_mediaPlayerStop},
    {"nodeAddChild","(JJ)V",(void*)OHB_nodeAddChild},
    {"nodeCreate","(I)J",(void*)OHB_nodeCreate},
    {"nodeDispose","(J)V",(void*)OHB_nodeDispose},
    {"nodeInsertChildAt","(JJI)V",(void*)OHB_nodeInsertChildAt},
    {"nodeMarkDirty","(JI)V",(void*)OHB_nodeMarkDirty},
    {"nodeRegisterEvent","(JII)I",(void*)OHB_nodeRegisterEvent},
    {"nodeRemoveChild","(JJ)V",(void*)OHB_nodeRemoveChild},
    {"nodeSetAttrColor","(JII)I",(void*)OHB_nodeSetAttrColor},
    {"nodeSetAttrFloat","(JIFFFFI)I",(void*)OHB_nodeSetAttrFloat},
    {"nodeSetAttrInt","(JII)I",(void*)OHB_nodeSetAttrInt},
    {"nodeSetAttrString","(JILjava/lang/String;)I",(void*)OHB_nodeSetAttrString},
    {"nodeUnregisterEvent","(JI)V",(void*)OHB_nodeUnregisterEvent},
    {"notificationAddSlot","(Ljava/lang/String;Ljava/lang/String;I)V",(void*)OHB_notificationAddSlot},
    {"notificationCancel","(I)V",(void*)OHB_notificationCancel},
    {"notificationPublish","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V",(void*)OHB_notificationPublish},
    {"pathAddCircle","(JFFFI)V",(void*)OHB_pathAddCircle},
    {"pathAddRect","(JFFFFI)V",(void*)OHB_pathAddRect},
    {"pathClose","(J)V",(void*)OHB_pathClose},
    {"pathCreate","()J",(void*)OHB_pathCreate},
    {"pathCubicTo","(JFFFFFF)V",(void*)OHB_pathCubicTo},
    {"pathDestroy","(J)V",(void*)OHB_pathDestroy},
    {"pathLineTo","(JFF)V",(void*)OHB_pathLineTo},
    {"pathMoveTo","(JFF)V",(void*)OHB_pathMoveTo},
    {"pathQuadTo","(JFFFF)V",(void*)OHB_pathQuadTo},
    {"pathReset","(J)V",(void*)OHB_pathReset},
    {"penCreate","()J",(void*)OHB_penCreate},
    {"penDestroy","(J)V",(void*)OHB_penDestroy},
    {"penSetAntiAlias","(JZ)V",(void*)OHB_penSetAntiAlias},
    {"penSetCap","(JI)V",(void*)OHB_penSetCap},
    {"penSetColor","(JI)V",(void*)OHB_penSetColor},
    {"penSetJoin","(JI)V",(void*)OHB_penSetJoin},
    {"penSetWidth","(JF)V",(void*)OHB_penSetWidth},
    {"preferencesClear","(J)V",(void*)OHB_preferencesClear},
    {"preferencesClose","(J)V",(void*)OHB_preferencesClose},
    {"preferencesFlush","(J)V",(void*)OHB_preferencesFlush},
    {"preferencesGetBoolean","(JLjava/lang/String;Z)Z",(void*)OHB_preferencesGetBoolean},
    {"preferencesGetFloat","(JLjava/lang/String;F)F",(void*)OHB_preferencesGetFloat},
    {"preferencesGetInt","(JLjava/lang/String;I)I",(void*)OHB_preferencesGetInt},
    {"preferencesGetLong","(JLjava/lang/String;J)J",(void*)OHB_preferencesGetLong},
    {"preferencesGetString","(JLjava/lang/String;Ljava/lang/String;)Ljava/lang/String;",(void*)OHB_preferencesGetString},
    {"preferencesOpen","(Ljava/lang/String;)J",(void*)OHB_preferencesOpen},
    {"preferencesPutBoolean","(JLjava/lang/String;Z)V",(void*)OHB_preferencesPutBoolean},
    {"preferencesPutFloat","(JLjava/lang/String;F)V",(void*)OHB_preferencesPutFloat},
    {"preferencesPutInt","(JLjava/lang/String;I)V",(void*)OHB_preferencesPutInt},
    {"preferencesPutLong","(JLjava/lang/String;J)V",(void*)OHB_preferencesPutLong},
    {"preferencesPutString","(JLjava/lang/String;Ljava/lang/String;)V",(void*)OHB_preferencesPutString},
    {"preferencesRemove","(JLjava/lang/String;)V",(void*)OHB_preferencesRemove},
    {"rdbStoreBeginTransaction","(J)V",(void*)OHB_rdbStoreBeginTransaction},
    {"rdbStoreClose","(J)V",(void*)OHB_rdbStoreClose},
    {"rdbStoreCommit","(J)V",(void*)OHB_rdbStoreCommit},
    {"rdbStoreDelete","(JLjava/lang/String;Ljava/lang/String;[Ljava/lang/String;)I",(void*)OHB_rdbStoreDelete},
    {"rdbStoreExecSQL","(JLjava/lang/String;)V",(void*)OHB_rdbStoreExecSQL},
    {"rdbStoreInsert","(JLjava/lang/String;Ljava/lang/String;)J",(void*)OHB_rdbStoreInsert},
    {"rdbStoreOpen","(Ljava/lang/String;I)J",(void*)OHB_rdbStoreOpen},
    {"rdbStoreQuery","(JLjava/lang/String;[Ljava/lang/String;)J",(void*)OHB_rdbStoreQuery},
    {"rdbStoreRollback","(J)V",(void*)OHB_rdbStoreRollback},
    {"rdbStoreUpdate","(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)I",(void*)OHB_rdbStoreUpdate},
    {"reminderCancel","(I)V",(void*)OHB_reminderCancel},
    {"reminderScheduleTimer","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",(void*)OHB_reminderScheduleTimer},
    {"resultSetClose","(J)V",(void*)OHB_resultSetClose},
    {"resultSetGetBlob","(JI)[B",(void*)OHB_resultSetGetBlob},
    {"resultSetGetColumnCount","(J)I",(void*)OHB_resultSetGetColumnCount},
    {"resultSetGetColumnIndex","(JLjava/lang/String;)I",(void*)OHB_resultSetGetColumnIndex},
    {"resultSetGetColumnName","(JI)Ljava/lang/String;",(void*)OHB_resultSetGetColumnName},
    {"resultSetGetDouble","(JI)D",(void*)OHB_resultSetGetDouble},
    {"resultSetGetFloat","(JI)F",(void*)OHB_resultSetGetFloat},
    {"resultSetGetInt","(JI)I",(void*)OHB_resultSetGetInt},
    {"resultSetGetLong","(JI)J",(void*)OHB_resultSetGetLong},
    {"resultSetGetRowCount","(J)I",(void*)OHB_resultSetGetRowCount},
    {"resultSetGetString","(JI)Ljava/lang/String;",(void*)OHB_resultSetGetString},
    {"resultSetGoToFirstRow","(J)Z",(void*)OHB_resultSetGoToFirstRow},
    {"resultSetGoToNextRow","(J)Z",(void*)OHB_resultSetGoToNextRow},
    {"resultSetIsNull","(JI)Z",(void*)OHB_resultSetIsNull},
    {"sensorGetData","(I)[F",(void*)OHB_sensorGetData},
    {"sensorIsAvailable","(I)Z",(void*)OHB_sensorIsAvailable},
    {"showToast","(Ljava/lang/String;I)V",(void*)OHB_showToast},
    {"startAbility","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_startAbility},
    {"surfaceCreate","(JII)J",(void*)OHB_surfaceCreate},
    {"surfaceDestroy","(J)V",(void*)OHB_surfaceDestroy},
    {"surfaceFlush","(J)I",(void*)OHB_surfaceFlush},
    {"surfaceGetCanvas","(J)J",(void*)OHB_surfaceGetCanvas},
    {"surfaceResize","(JII)V",(void*)OHB_surfaceResize},
    {"telephonyGetDeviceId","()Ljava/lang/String;",(void*)OHB_telephonyGetDeviceId},
    {"telephonyGetLine1Number","()Ljava/lang/String;",(void*)OHB_telephonyGetLine1Number},
    {"telephonyGetNetworkOperatorName","()Ljava/lang/String;",(void*)OHB_telephonyGetNetworkOperatorName},
    {"telephonyGetNetworkType","()I",(void*)OHB_telephonyGetNetworkType},
    {"telephonyGetPhoneType","()I",(void*)OHB_telephonyGetPhoneType},
    {"telephonyGetSimState","()I",(void*)OHB_telephonyGetSimState},
    {"terminateSelf","()V",(void*)OHB_terminateSelf},
    {"vibratorCancel","()V",(void*)OHB_vibratorCancel},
    {"vibratorHasVibrator","()Z",(void*)OHB_vibratorHasVibrator},
    {"vibratorVibrate","(J)V",(void*)OHB_vibratorVibrate},
    {"wifiGetFrequency","()I",(void*)OHB_wifiGetFrequency},
    {"wifiGetLinkSpeed","()I",(void*)OHB_wifiGetLinkSpeed},
    {"wifiGetRssi","()I",(void*)OHB_wifiGetRssi},
    {"wifiGetSSID","()Ljava/lang/String;",(void*)OHB_wifiGetSSID},
    {"wifiGetState","()I",(void*)OHB_wifiGetState},
    {"wifiIsEnabled","()Z",(void*)OHB_wifiIsEnabled},
    {"wifiSetEnabled","(Z)Z",(void*)OHB_wifiSetEnabled},
  };
  int n=sizeof(m)/sizeof(m[0]);
  if((*e)->RegisterNatives(e,c,m,n)!=0){(*e)->ExceptionClear(e);
    for(int i=0;i<n;i++){if((*e)->RegisterNatives(e,c,&m[i],1)!=0)(*e)->ExceptionClear(e);}}
  fprintf(stderr, "[OHBridge ARM64] %d methods\n",n); return JNI_VERSION_1_6;}
