#include <jni.h>
#include <string.h>
#include <stdlib.h>

/* Stub implementations for com.android.icu.util.Icu4cMetadata native methods */

static jstring Icu4cMetadata_getTzdbVersion(JNIEnv* env, jclass clazz) {
    return (*env)->NewStringUTF(env, "2021a");
}

static jstring Icu4cMetadata_getCldrVersion(JNIEnv* env, jclass clazz) {
    return (*env)->NewStringUTF(env, "39");
}

static jstring Icu4cMetadata_getIcuVersion(JNIEnv* env, jclass clazz) {
    return (*env)->NewStringUTF(env, "69.1");
}

static jstring Icu4cMetadata_getUnicodeVersion(JNIEnv* env, jclass clazz) {
    return (*env)->NewStringUTF(env, "13.0");
}

/* ==================== NativeConverter stubs ==================== */

/* Supported charsets - name, canonical ICU name, aliases */
typedef struct {
    const char* name;
    const char* icuName;
    const char* aliases[8];
    int aliasCount;
    float avgBytesPerChar;
    float aveCharsPerByte;
    int maxBytesPerChar;
} CharsetInfo;

static const CharsetInfo CHARSETS[] = {
    {"UTF-8",       "UTF-8",       {"unicode-1-1-utf-8", "UTF8", "utf8", NULL}, 3, 1.1f, 1.0f, 4},
    {"US-ASCII",    "US-ASCII",    {"ASCII", "ascii", "us-ascii", "iso-ir-6", NULL}, 4, 1.0f, 1.0f, 1},
    {"ISO-8859-1",  "ISO-8859-1",  {"latin1", "iso_8859_1", "8859_1", "ISO8859-1", "ISO8859_1", NULL}, 5, 1.0f, 1.0f, 1},
    {"UTF-16",      "UTF-16",      {"utf16", "UTF_16", NULL}, 2, 2.0f, 0.5f, 4},
    {"UTF-16BE",    "UTF-16BE",    {"utf16be", "UTF_16BE", NULL}, 2, 2.0f, 0.5f, 2},
    {"UTF-16LE",    "UTF-16LE",    {"utf16le", "UTF_16LE", NULL}, 2, 2.0f, 0.5f, 2},
    {"UTF-32",      "UTF-32",      {"utf32", "UTF_32", NULL}, 2, 4.0f, 0.25f, 4},
    {"UTF-32BE",    "UTF-32BE",    {"utf32be", NULL}, 1, 4.0f, 0.25f, 4},
    {"UTF-32LE",    "UTF-32LE",    {"utf32le", NULL}, 1, 4.0f, 0.25f, 4},
};
#define NUM_CHARSETS (sizeof(CHARSETS) / sizeof(CHARSETS[0]))

static int strcasecmp_ascii(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a;
        char cb = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *a - *b;
}

static const CharsetInfo* findCharset(const char* name) {
    for (int i = 0; i < (int)NUM_CHARSETS; i++) {
        if (strcasecmp_ascii(name, CHARSETS[i].name) == 0) return &CHARSETS[i];
        if (strcasecmp_ascii(name, CHARSETS[i].icuName) == 0) return &CHARSETS[i];
        for (int j = 0; j < CHARSETS[i].aliasCount; j++) {
            if (CHARSETS[i].aliases[j] && strcasecmp_ascii(name, CHARSETS[i].aliases[j]) == 0)
                return &CHARSETS[i];
        }
    }
    return NULL;
}

/* charsetForName(String) -> Charset */
static jobject NativeConverter_charsetForName(JNIEnv* env, jclass clazz, jstring jname) {
    if (!jname) return NULL;
    const char* name = (*env)->GetStringUTFChars(env, jname, NULL);
    const CharsetInfo* info = findCharset(name);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    if (!info) return NULL;

    /* Create CharsetICU(canonicalName, icuCanonName, aliases) */
    jclass charsetCls = (*env)->FindClass(env, "com/android/icu/charset/CharsetICU");
    if (!charsetCls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, charsetCls, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
    if (!ctor) {
        (*env)->DeleteLocalRef(env, charsetCls);
        return NULL;
    }

    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    jobjectArray aliases = (*env)->NewObjectArray(env, info->aliasCount, stringCls, NULL);
    for (int i = 0; i < info->aliasCount; i++) {
        (*env)->SetObjectArrayElement(env, aliases, i, (*env)->NewStringUTF(env, info->aliases[i]));
    }

    jobject result = (*env)->NewObject(env, charsetCls, ctor,
        (*env)->NewStringUTF(env, info->name),
        (*env)->NewStringUTF(env, info->icuName),
        aliases);

    (*env)->DeleteLocalRef(env, charsetCls);
    (*env)->DeleteLocalRef(env, stringCls);
    (*env)->DeleteLocalRef(env, aliases);
    return result;
}

/* getAvailableCharsetNames() -> String[] */
static jobjectArray NativeConverter_getAvailableCharsetNames(JNIEnv* env, jclass clazz) {
    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray(env, (jint)NUM_CHARSETS, stringCls, NULL);
    for (int i = 0; i < (int)NUM_CHARSETS; i++) {
        (*env)->SetObjectArrayElement(env, result, i, (*env)->NewStringUTF(env, CHARSETS[i].name));
    }
    (*env)->DeleteLocalRef(env, stringCls);
    return result;
}

/* openConverter(String) -> long (fake handle) */
static jlong NativeConverter_openConverter(JNIEnv* env, jclass clazz, jstring jname) {
    if (!jname) return 0;
    const char* name = (*env)->GetStringUTFChars(env, jname, NULL);
    const CharsetInfo* info = findCharset(name);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    if (!info) return 0;
    /* Use index + 1 as fake handle */
    return (jlong)(info - CHARSETS + 1);
}

/* closeConverter(long) */
static void NativeConverter_closeConverter(JNIEnv* env, jclass clazz, jlong handle) {
    /* no-op for fake handles */
}

/* getMaxBytesPerChar(long) */
static jint NativeConverter_getMaxBytesPerChar(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle < 1 || handle > (jlong)NUM_CHARSETS) return 1;
    return CHARSETS[handle - 1].maxBytesPerChar;
}

/* getAveBytesPerChar(long) */
static jfloat NativeConverter_getAveBytesPerChar(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle < 1 || handle > (jlong)NUM_CHARSETS) return 1.0f;
    return CHARSETS[handle - 1].avgBytesPerChar;
}

/* getAveCharsPerByte(long) */
static jfloat NativeConverter_getAveCharsPerByte(JNIEnv* env, jclass clazz, jlong handle) {
    if (handle < 1 || handle > (jlong)NUM_CHARSETS) return 1.0f;
    return CHARSETS[handle - 1].aveCharsPerByte;
}

/* contains(String, String) */
static jboolean NativeConverter_contains(JNIEnv* env, jclass clazz, jstring name1, jstring name2) {
    /* Simplified: UTF-8 contains everything, ASCII contains ASCII, etc. */
    return JNI_FALSE;
}

/* getSubstitutionBytes(long) -> byte[] */
static jbyteArray NativeConverter_getSubstitutionBytes(JNIEnv* env, jclass clazz, jlong handle) {
    jbyteArray result = (*env)->NewByteArray(env, 1);
    jbyte sub = '?';
    (*env)->SetByteArrayRegion(env, result, 0, 1, &sub);
    return result;
}

/* resetByteToChar / resetCharToByte */
static void NativeConverter_resetByteToChar(JNIEnv* env, jclass clazz, jlong handle) { }
static void NativeConverter_resetCharToByte(JNIEnv* env, jclass clazz, jlong handle) { }

/* getNativeFinalizer() -> long */
static jlong NativeConverter_getNativeFinalizer(JNIEnv* env, jclass clazz) {
    return 0; /* no native finalizer needed for stubs */
}

/* decode(long handle, byte[] input, int inEnd, char[] output, int outEnd, int[] data, boolean flush)
   DEX signature: (J[BI[CI[IZ)I */
static jint NativeConverter_decode(JNIEnv* env, jclass clazz, jlong handle,
                                    jbyteArray input, jint inEnd,
                                    jcharArray output, jint outEnd,
                                    jintArray data, jboolean flush) {
    jbyte* inBuf = (*env)->GetByteArrayElements(env, input, NULL);
    jchar* outBuf = (*env)->GetCharArrayElements(env, output, NULL);
    jint* d = (*env)->GetIntArrayElements(env, data, NULL);
    jint inPos = d[0];
    jint outPos = d[1];

    jint count = (inEnd - inPos) < (outEnd - outPos) ? (inEnd - inPos) : (outEnd - outPos);
    for (jint i = 0; i < count; i++) {
        outBuf[outPos + i] = (jchar)(inBuf[inPos + i] & 0xFF);
    }
    d[0] = inPos + count;
    d[1] = outPos + count;

    (*env)->ReleaseIntArrayElements(env, data, d, 0);
    (*env)->ReleaseCharArrayElements(env, output, outBuf, 0);
    (*env)->ReleaseByteArrayElements(env, input, inBuf, JNI_ABORT);
    return 0; /* CoderResult.UNDERFLOW */
}

/* encode(long handle, char[] input, int inEnd, byte[] output, int outEnd, int[] data, boolean flush)
   DEX signature: (J[CI[BI[IZ)I */
static jint NativeConverter_encode(JNIEnv* env, jclass clazz, jlong handle,
                                    jcharArray input, jint inEnd,
                                    jbyteArray output, jint outEnd,
                                    jintArray data, jboolean flush) {
    jchar* inBuf = (*env)->GetCharArrayElements(env, input, NULL);
    jbyte* outBuf = (*env)->GetByteArrayElements(env, output, NULL);
    jint* d = (*env)->GetIntArrayElements(env, data, NULL);
    jint inPos = d[0];
    jint outPos = d[1];

    jint count = (inEnd - inPos) < (outEnd - outPos) ? (inEnd - inPos) : (outEnd - outPos);
    for (jint i = 0; i < count; i++) {
        outBuf[outPos + i] = (jbyte)(inBuf[inPos + i] & 0xFF);
    }
    d[0] = inPos + count;
    d[1] = outPos + count;

    (*env)->ReleaseIntArrayElements(env, data, d, 0);
    (*env)->ReleaseByteArrayElements(env, output, outBuf, 0);
    (*env)->ReleaseCharArrayElements(env, input, inBuf, JNI_ABORT);
    return 0;
}

/* setCallbackDecode / setCallbackEncode */
static void NativeConverter_setCallbackDecode(JNIEnv* env, jclass clazz, jlong handle,
                                               jint onMalformed, jint onUnmappable, jstring subChars) { }
static void NativeConverter_setCallbackEncode(JNIEnv* env, jclass clazz, jlong handle,
                                               jint onMalformed, jint onUnmappable, jbyteArray subBytes) { }

/* ==================== JNI_OnLoad ==================== */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) return -1;

    /* Register Icu4cMetadata methods */
    {
        jclass cls = (*env)->FindClass(env, "com/android/icu/util/Icu4cMetadata");
        if (cls) {
            JNINativeMethod methods[] = {
                {"getTzdbVersion", "()Ljava/lang/String;", (void*)Icu4cMetadata_getTzdbVersion},
                {"getCldrVersion", "()Ljava/lang/String;", (void*)Icu4cMetadata_getCldrVersion},
                {"getIcuVersion", "()Ljava/lang/String;", (void*)Icu4cMetadata_getIcuVersion},
                {"getUnicodeVersion", "()Ljava/lang/String;", (void*)Icu4cMetadata_getUnicodeVersion},
            };
            (*env)->RegisterNatives(env, cls, methods, 4);
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* Register NativeConverter methods */
    {
        jclass cls = (*env)->FindClass(env, "com/android/icu/charset/NativeConverter");
        if (cls) {
            JNINativeMethod methods[] = {
                {"charsetForName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;", (void*)NativeConverter_charsetForName},
                {"getAvailableCharsetNames", "()[Ljava/lang/String;", (void*)NativeConverter_getAvailableCharsetNames},
                {"openConverter", "(Ljava/lang/String;)J", (void*)NativeConverter_openConverter},
                {"closeConverter", "(J)V", (void*)NativeConverter_closeConverter},
                {"getMaxBytesPerChar", "(J)I", (void*)NativeConverter_getMaxBytesPerChar},
                {"getAveBytesPerChar", "(J)F", (void*)NativeConverter_getAveBytesPerChar},
                {"getAveCharsPerByte", "(J)F", (void*)NativeConverter_getAveCharsPerByte},
                {"contains", "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)NativeConverter_contains},
                {"getSubstitutionBytes", "(J)[B", (void*)NativeConverter_getSubstitutionBytes},
                {"resetByteToChar", "(J)V", (void*)NativeConverter_resetByteToChar},
                {"resetCharToByte", "(J)V", (void*)NativeConverter_resetCharToByte},
                {"getNativeFinalizer", "()J", (void*)NativeConverter_getNativeFinalizer},
                {"decode", "(J[BI[CI[IZ)I", (void*)NativeConverter_decode},
                {"encode", "(J[CI[BI[IZ)I", (void*)NativeConverter_encode},
                {"setCallbackDecode", "(JIILjava/lang/String;)V", (void*)NativeConverter_setCallbackDecode},
                {"setCallbackEncode", "(JII[B)V", (void*)NativeConverter_setCallbackEncode},
            };
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    return JNI_VERSION_1_6;
}
