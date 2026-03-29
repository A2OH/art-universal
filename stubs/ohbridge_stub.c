/*
 * OHBridge JNI stub — stdout pipe display list mode.
 * Writes Canvas ops to local buffer, flushes as [4-byte LE size][ops] to pipe.
 * Host reads from process.inputStream → replays on SurfaceView.
 *
 * On init, saves stdout fd for binary pipe and redirects stdout→stderr
 * so Java System.out and stray printf don't corrupt the binary stream.
 */
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define DLIST_MAX (512*1024)

static unsigned char dlist_buf[DLIST_MAX];
static int dlist_pos = 0;
static int pipe_fd = -1;   /* saved stdout fd for binary pipe */
static const int DLIST_MAGIC = 0x444C5354; /* "DLST" */
static JavaVM* g_vm = NULL;

static void emit1(unsigned char v) { if(dlist_pos<DLIST_MAX-64) dlist_buf[dlist_pos++]=v; }
static void emit4(const void* v) { if(dlist_pos+4<=DLIST_MAX-64){memcpy(dlist_buf+dlist_pos,v,4);dlist_pos+=4;} }
static void emitf(float v) { emit4(&v); }
static void emiti(int v) { emit4(&v); }
static void emit2(short v) { if(dlist_pos+2<=DLIST_MAX-64){memcpy(dlist_buf+dlist_pos,&v,2);dlist_pos+=2;} }

enum { OP_COLOR=1,OP_RECT=2,OP_TEXT=3,OP_LINE=4,OP_SAVE=5,OP_RESTORE=6,OP_TRANSLATE=7,OP_CLIP=8,OP_RRECT=9,OP_CIRCLE=10 };

#define MAX_H 256
static int h_colors[MAX_H];
static float h_fontsz[MAX_H];
static int h_next = 1;
static int idx(long h) { return (int)(h & 0xFF); }

/* Write all bytes to fd, handling partial writes */
static void write_all(int fd, const void* buf, int len) {
    const unsigned char* p = (const unsigned char*)buf;
    while (len > 0) {
        int n = write(fd, p, len);
        if (n <= 0) break;
        p += n;
        len -= n;
    }
}

/* === JNI exports (Java_com_ohos_shim_bridge_OHBridge_*) === */
#define JF(name) Java_com_ohos_shim_bridge_OHBridge_##name

JNIEXPORT jint JNICALL JF(arkuiInit)(JNIEnv* e, jclass c) {
    fprintf(stderr, "[OHBridge] pipe mode arkuiInit (pipe_fd=%d)\n", pipe_fd);
    return 0;
}
JNIEXPORT jlong JNICALL JF(surfaceCreate)(JNIEnv* e, jclass c, jlong u, jint w, jint h) { return 1; }
JNIEXPORT jlong JNICALL JF(surfaceGetCanvas)(JNIEnv* e, jclass c, jlong s) { dlist_pos=0; return 1; }
JNIEXPORT jint JNICALL JF(surfaceFlush)(JNIEnv* e, jclass c, jlong s) {
    if(pipe_fd<0) return -1;
    int size = dlist_pos;
    write_all(pipe_fd, &DLIST_MAGIC, 4);
    write_all(pipe_fd, &size, 4);
    write_all(pipe_fd, dlist_buf, size);
    return 0;
}
JNIEXPORT void JNICALL JF(surfaceDestroy)(JNIEnv* e, jclass c, jlong s) {}
JNIEXPORT void JNICALL JF(surfaceResize)(JNIEnv* e, jclass c, jlong s, jint w, jint h) {}

JNIEXPORT jlong JNICALL JF(canvasCreate)(JNIEnv* e, jclass c, jlong b) { return 1; }
JNIEXPORT void JNICALL JF(canvasDestroy)(JNIEnv* e, jclass c, jlong cn) {}
JNIEXPORT void JNICALL JF(canvasDrawColor)(JNIEnv* e, jclass c, jlong cn, jint col) { emit1(OP_COLOR); emiti(col); }
JNIEXPORT void JNICALL JF(canvasDrawRect)(JNIEnv* e, jclass c, jlong cn, jfloat l, jfloat t, jfloat r, jfloat b2, jlong pen, jlong brush) {
    emit1(OP_RECT); emitf(l); emitf(t); emitf(r); emitf(b2); emiti(h_colors[idx(brush>0?brush:pen)]);
}
JNIEXPORT void JNICALL JF(canvasDrawRoundRect)(JNIEnv* e, jclass c, jlong cn, jfloat l, jfloat t, jfloat r, jfloat b2, jfloat rx, jfloat ry, jlong pen, jlong brush) {
    emit1(OP_RRECT); emitf(l); emitf(t); emitf(r); emitf(b2); emitf(rx); emitf(ry); emiti(h_colors[idx(brush>0?brush:pen)]);
}
JNIEXPORT void JNICALL JF(canvasDrawCircle)(JNIEnv* e, jclass c, jlong cn, jfloat cx, jfloat cy, jfloat r, jlong pen, jlong brush) {
    emit1(OP_CIRCLE); emitf(cx); emitf(cy); emitf(r); emiti(h_colors[idx(brush>0?brush:pen)]);
}
JNIEXPORT void JNICALL JF(canvasDrawLine)(JNIEnv* e, jclass c, jlong cn, jfloat x1, jfloat y1, jfloat x2, jfloat y2, jlong pen) {
    emit1(OP_LINE); emitf(x1); emitf(y1); emitf(x2); emitf(y2); emiti(h_colors[idx(pen)]); emitf(1.0f);
}
JNIEXPORT void JNICALL JF(canvasDrawText)(JNIEnv* e, jclass c, jlong cn, jstring text, jfloat x, jfloat y, jlong font, jlong pen, jlong brush) {
    if(!text) return;
    const char* u = (*e)->GetStringUTFChars(e,text,0);
    int len = u ? strlen(u) : 0;
    if(len>0 && dlist_pos+19+len<DLIST_MAX-64) {
        emit1(OP_TEXT); emitf(x); emitf(y); emitf(h_fontsz[idx(font)]);
        emiti(h_colors[idx(pen>0?pen:brush)]); emit2((short)len);
        memcpy(dlist_buf+dlist_pos,u,len); dlist_pos+=len;
    }
    if(u) (*e)->ReleaseStringUTFChars(e,text,u);
}
JNIEXPORT void JNICALL JF(canvasSave)(JNIEnv* e, jclass c, jlong cn) { emit1(OP_SAVE); }
JNIEXPORT void JNICALL JF(canvasRestore)(JNIEnv* e, jclass c, jlong cn) { emit1(OP_RESTORE); }
JNIEXPORT void JNICALL JF(canvasTranslate)(JNIEnv* e, jclass c, jlong cn, jfloat dx, jfloat dy) { emit1(OP_TRANSLATE); emitf(dx); emitf(dy); }
JNIEXPORT void JNICALL JF(canvasScale)(JNIEnv* e, jclass c, jlong cn, jfloat sx, jfloat sy) {}
JNIEXPORT void JNICALL JF(canvasClipRect)(JNIEnv* e, jclass c, jlong cn, jfloat l, jfloat t, jfloat r, jfloat b2) { emit1(OP_CLIP); emitf(l); emitf(t); emitf(r); emitf(b2); }
JNIEXPORT void JNICALL JF(canvasDrawPath)(JNIEnv* e, jclass c, jlong cn, jlong path, jlong pen, jlong brush) {}
JNIEXPORT void JNICALL JF(canvasDrawBitmap)(JNIEnv* e, jclass c, jlong cn, jlong bmp, jfloat x, jfloat y) {}
JNIEXPORT void JNICALL JF(canvasConcat)(JNIEnv* e, jclass c, jlong cn, jfloatArray m) {}
JNIEXPORT void JNICALL JF(canvasRotate)(JNIEnv* e, jclass c, jlong cn, jfloat d, jfloat px, jfloat py) {}
JNIEXPORT void JNICALL JF(canvasClipPath)(JNIEnv* e, jclass c, jlong cn, jlong path) {}
JNIEXPORT void JNICALL JF(canvasDrawArc)(JNIEnv* e, jclass c, jlong cn, jfloat l, jfloat t, jfloat r, jfloat b2, jfloat sa, jfloat sw, jboolean uc, jlong pen, jlong brush) {}
JNIEXPORT void JNICALL JF(canvasDrawOval)(JNIEnv* e, jclass c, jlong cn, jfloat l, jfloat t, jfloat r, jfloat b2, jlong pen, jlong brush) {}

JNIEXPORT jlong JNICALL JF(penCreate)(JNIEnv* e, jclass c) { int i=h_next++; if(i>=MAX_H)i=h_next=1; h_colors[i]=0xFF000000; return i; }
JNIEXPORT void JNICALL JF(penSetColor)(JNIEnv* e, jclass c, jlong p, jint col) { h_colors[idx(p)]=col; }
JNIEXPORT void JNICALL JF(penSetWidth)(JNIEnv* e, jclass c, jlong p, jfloat w) {}
JNIEXPORT void JNICALL JF(penSetAntiAlias)(JNIEnv* e, jclass c, jlong p, jboolean aa) {}
JNIEXPORT void JNICALL JF(penSetCap)(JNIEnv* e, jclass c, jlong p, jint cap) {}
JNIEXPORT void JNICALL JF(penSetJoin)(JNIEnv* e, jclass c, jlong p, jint j) {}
JNIEXPORT void JNICALL JF(penDestroy)(JNIEnv* e, jclass c, jlong p) {}
JNIEXPORT jlong JNICALL JF(brushCreate)(JNIEnv* e, jclass c) { int i=h_next++; if(i>=MAX_H)i=h_next=1; h_colors[i]=0xFF000000; return i; }
JNIEXPORT void JNICALL JF(brushSetColor)(JNIEnv* e, jclass c, jlong b, jint col) { h_colors[idx(b)]=col; }
JNIEXPORT void JNICALL JF(brushDestroy)(JNIEnv* e, jclass c, jlong b) {}
JNIEXPORT void JNICALL JF(brushSetAntiAlias)(JNIEnv* e, jclass c, jlong b, jboolean aa) {}

JNIEXPORT jlong JNICALL JF(fontCreate)(JNIEnv* e, jclass c) { int i=h_next++; if(i>=MAX_H)i=h_next=1; h_fontsz[i]=16.0f; return i; }
JNIEXPORT void JNICALL JF(fontSetSize)(JNIEnv* e, jclass c, jlong f, jfloat sz) { h_fontsz[idx(f)]=sz; }
JNIEXPORT jfloat JNICALL JF(fontMeasureText)(JNIEnv* e, jclass c, jlong f, jstring s) {
    if(!s) return 0;
    const char* u=(*e)->GetStringUTFChars(e,s,0);
    float w=u?strlen(u)*h_fontsz[idx(f)]*0.55f:0;
    if(u)(*e)->ReleaseStringUTFChars(e,s,u);
    return w;
}
JNIEXPORT void JNICALL JF(fontDestroy)(JNIEnv* e, jclass c, jlong f) {}
JNIEXPORT jfloatArray JNICALL JF(fontGetMetrics)(JNIEnv* e, jclass c, jlong f) {
    jfloatArray a=(*e)->NewFloatArray(e,4);
    float s=h_fontsz[idx(f)], m[4]={-s*0.8f,s*0.2f,0,s};
    (*e)->SetFloatArrayRegion(e,a,0,4,m);
    return a;
}

JNIEXPORT jlong JNICALL JF(bitmapCreate)(JNIEnv* e, jclass c, jint w, jint h, jint fmt) { return 1; }
JNIEXPORT void JNICALL JF(bitmapDestroy)(JNIEnv* e, jclass c, jlong b) {}
JNIEXPORT jint JNICALL JF(bitmapGetWidth)(JNIEnv* e, jclass c, jlong b) { return 480; }
JNIEXPORT jint JNICALL JF(bitmapGetHeight)(JNIEnv* e, jclass c, jlong b) { return 800; }
JNIEXPORT void JNICALL JF(bitmapSetPixel)(JNIEnv* e, jclass c, jlong b, jint x, jint y, jint col) {}
JNIEXPORT jint JNICALL JF(bitmapGetPixel)(JNIEnv* e, jclass c, jlong b, jint x, jint y) { return 0; }

JNIEXPORT jlong JNICALL JF(pathCreate)(JNIEnv* e, jclass c) { return 1; }
JNIEXPORT void JNICALL JF(pathDestroy)(JNIEnv* e, jclass c, jlong p) {}
JNIEXPORT void JNICALL JF(pathMoveTo)(JNIEnv* e, jclass c, jlong p, jfloat x, jfloat y) {}
JNIEXPORT void JNICALL JF(pathLineTo)(JNIEnv* e, jclass c, jlong p, jfloat x, jfloat y) {}
JNIEXPORT void JNICALL JF(pathClose)(JNIEnv* e, jclass c, jlong p) {}
JNIEXPORT void JNICALL JF(pathReset)(JNIEnv* e, jclass c, jlong p) {}
JNIEXPORT void JNICALL JF(pathQuadTo)(JNIEnv* e, jclass c, jlong p, jfloat x1, jfloat y1, jfloat x2, jfloat y2) {}
JNIEXPORT void JNICALL JF(pathCubicTo)(JNIEnv* e, jclass c, jlong p, jfloat x1, jfloat y1, jfloat x2, jfloat y2, jfloat x3, jfloat y3) {}
JNIEXPORT void JNICALL JF(pathAddRect)(JNIEnv* e, jclass c, jlong p, jfloat l, jfloat t, jfloat r, jfloat b, jint dir) {}
JNIEXPORT void JNICALL JF(pathAddCircle)(JNIEnv* e, jclass c, jlong p, jfloat cx, jfloat cy, jfloat r, jint dir) {}

/* === Logging & device info stubs === */
JNIEXPORT void JNICALL JF(logDebug)(JNIEnv* e, jclass c, jstring tag, jstring msg) {
    if(!tag||!msg) return;
    const char* t=(*e)->GetStringUTFChars(e,tag,0);
    const char* m=(*e)->GetStringUTFChars(e,msg,0);
    fprintf(stderr,"D/%s: %s\n",t?t:"?",m?m:"");
    if(t)(*e)->ReleaseStringUTFChars(e,tag,t);
    if(m)(*e)->ReleaseStringUTFChars(e,msg,m);
}
JNIEXPORT void JNICALL JF(logInfo)(JNIEnv* e, jclass c, jstring tag, jstring msg) {
    if(!tag||!msg) return;
    const char* t=(*e)->GetStringUTFChars(e,tag,0);
    const char* m=(*e)->GetStringUTFChars(e,msg,0);
    fprintf(stderr,"I/%s: %s\n",t?t:"?",m?m:"");
    if(t)(*e)->ReleaseStringUTFChars(e,tag,t);
    if(m)(*e)->ReleaseStringUTFChars(e,msg,m);
}
JNIEXPORT void JNICALL JF(logError)(JNIEnv* e, jclass c, jstring tag, jstring msg) {
    if(!tag||!msg) return;
    const char* t=(*e)->GetStringUTFChars(e,tag,0);
    const char* m=(*e)->GetStringUTFChars(e,msg,0);
    fprintf(stderr,"E/%s: %s\n",t?t:"?",m?m:"");
    if(t)(*e)->ReleaseStringUTFChars(e,tag,t);
    if(m)(*e)->ReleaseStringUTFChars(e,msg,m);
}
JNIEXPORT jstring JNICALL JF(getDeviceBrand)(JNIEnv* e, jclass c) { return (*e)->NewStringUTF(e,"Westlake"); }
JNIEXPORT jstring JNICALL JF(getDeviceModel)(JNIEnv* e, jclass c) { return (*e)->NewStringUTF(e,"VM"); }
JNIEXPORT jstring JNICALL JF(getOSVersion)(JNIEnv* e, jclass c) { return (*e)->NewStringUTF(e,"11"); }
JNIEXPORT jint JNICALL JF(getSDKVersion)(JNIEnv* e, jclass c) { return 30; }

/* === Registration table === */
static JNINativeMethod methods[] = {
    {"arkuiInit","()I",(void*)JF(arkuiInit)},
    {"logDebug","(Ljava/lang/String;Ljava/lang/String;)V",(void*)JF(logDebug)},
    {"logInfo","(Ljava/lang/String;Ljava/lang/String;)V",(void*)JF(logInfo)},
    {"logError","(Ljava/lang/String;Ljava/lang/String;)V",(void*)JF(logError)},
    {"getDeviceBrand","()Ljava/lang/String;",(void*)JF(getDeviceBrand)},
    {"getDeviceModel","()Ljava/lang/String;",(void*)JF(getDeviceModel)},
    {"getOSVersion","()Ljava/lang/String;",(void*)JF(getOSVersion)},
    {"getSDKVersion","()I",(void*)JF(getSDKVersion)},
    {"surfaceCreate","(JII)J",(void*)JF(surfaceCreate)},
    {"surfaceGetCanvas","(J)J",(void*)JF(surfaceGetCanvas)},
    {"surfaceFlush","(J)I",(void*)JF(surfaceFlush)},
    {"surfaceDestroy","(J)V",(void*)JF(surfaceDestroy)},
    {"surfaceResize","(JII)V",(void*)JF(surfaceResize)},
    {"canvasCreate","(J)J",(void*)JF(canvasCreate)},{"canvasDestroy","(J)V",(void*)JF(canvasDestroy)},
    {"canvasDrawColor","(JI)V",(void*)JF(canvasDrawColor)},
    {"canvasDrawRect","(JFFFFJJ)V",(void*)JF(canvasDrawRect)},
    {"canvasDrawRoundRect","(JFFFFFFJJ)V",(void*)JF(canvasDrawRoundRect)},
    {"canvasDrawCircle","(JFFFJJ)V",(void*)JF(canvasDrawCircle)},
    {"canvasDrawLine","(JFFFFJ)V",(void*)JF(canvasDrawLine)},
    {"canvasDrawText","(JLjava/lang/String;FFJJJ)V",(void*)JF(canvasDrawText)},
    {"canvasSave","(J)V",(void*)JF(canvasSave)},{"canvasRestore","(J)V",(void*)JF(canvasRestore)},
    {"canvasTranslate","(JFF)V",(void*)JF(canvasTranslate)},{"canvasScale","(JFF)V",(void*)JF(canvasScale)},
    {"canvasClipRect","(JFFFF)V",(void*)JF(canvasClipRect)},
    {"canvasDrawPath","(JJJJ)V",(void*)JF(canvasDrawPath)},
    {"canvasDrawBitmap","(JJFF)V",(void*)JF(canvasDrawBitmap)},
    {"canvasConcat","(J[F)V",(void*)JF(canvasConcat)},
    {"canvasRotate","(JFFF)V",(void*)JF(canvasRotate)},
    {"canvasClipPath","(JJ)V",(void*)JF(canvasClipPath)},
    {"canvasDrawArc","(JFFFFFFZJJ)V",(void*)JF(canvasDrawArc)},
    {"canvasDrawOval","(JFFFFJJ)V",(void*)JF(canvasDrawOval)},
    {"penCreate","()J",(void*)JF(penCreate)},{"penSetColor","(JI)V",(void*)JF(penSetColor)},
    {"penSetWidth","(JF)V",(void*)JF(penSetWidth)},{"penSetAntiAlias","(JZ)V",(void*)JF(penSetAntiAlias)},
    {"penSetCap","(JI)V",(void*)JF(penSetCap)},{"penSetJoin","(JI)V",(void*)JF(penSetJoin)},
    {"penDestroy","(J)V",(void*)JF(penDestroy)},
    {"brushCreate","()J",(void*)JF(brushCreate)},{"brushSetColor","(JI)V",(void*)JF(brushSetColor)},
    {"brushDestroy","(J)V",(void*)JF(brushDestroy)},{"brushSetAntiAlias","(JZ)V",(void*)JF(brushSetAntiAlias)},
    {"fontCreate","()J",(void*)JF(fontCreate)},{"fontSetSize","(JF)V",(void*)JF(fontSetSize)},
    {"fontMeasureText","(JLjava/lang/String;)F",(void*)JF(fontMeasureText)},
    {"fontDestroy","(J)V",(void*)JF(fontDestroy)},{"fontGetMetrics","(J)[F",(void*)JF(fontGetMetrics)},
    {"bitmapCreate","(III)J",(void*)JF(bitmapCreate)},{"bitmapDestroy","(J)V",(void*)JF(bitmapDestroy)},
    {"bitmapGetWidth","(J)I",(void*)JF(bitmapGetWidth)},{"bitmapGetHeight","(J)I",(void*)JF(bitmapGetHeight)},
    {"bitmapSetPixel","(JIII)V",(void*)JF(bitmapSetPixel)},{"bitmapGetPixel","(JII)I",(void*)JF(bitmapGetPixel)},
    {"pathCreate","()J",(void*)JF(pathCreate)},{"pathDestroy","(J)V",(void*)JF(pathDestroy)},
    {"pathMoveTo","(JFF)V",(void*)JF(pathMoveTo)},{"pathLineTo","(JFF)V",(void*)JF(pathLineTo)},
    {"pathClose","(J)V",(void*)JF(pathClose)},{"pathReset","(J)V",(void*)JF(pathReset)},
    {"pathQuadTo","(JFFFF)V",(void*)JF(pathQuadTo)},{"pathCubicTo","(JFFFFFF)V",(void*)JF(pathCubicTo)},
    {"pathAddRect","(JFFFFI)V",(void*)JF(pathAddRect)},{"pathAddCircle","(JFFFI)V",(void*)JF(pathAddCircle)},
};

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    /* Save stdout for binary pipe ASAP, redirect stdout→stderr */
    if (pipe_fd < 0) {
        pipe_fd = dup(STDOUT_FILENO);
        dup2(STDERR_FILENO, STDOUT_FILENO);
    }
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_VERSION_1_6;
    jclass cls = (*env)->FindClass(env, "com/ohos/shim/bridge/OHBridge");
    int ok = 0, count = sizeof(methods)/sizeof(methods[0]);
    if (cls) {
        for (int i = 0; i < count; i++) {
            if ((*env)->RegisterNatives(env, cls, &methods[i], 1) == 0) ok++;
            else (*env)->ExceptionClear(env);
        }
    } else { (*env)->ExceptionClear(env); }
    fprintf(stderr, "[OHBridge] JNI_OnLoad (pipe stub) %d/%d registered, pipe_fd=%d\n", ok, count, pipe_fd);
    return JNI_VERSION_1_6;
}
