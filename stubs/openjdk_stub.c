#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <zlib.h>
#include <regex.h>

/* Register native methods one at a time, skipping failures */
static int registerNativesOrSkip(JNIEnv* env, jclass clazz,
                                  const JNINativeMethod* methods, int numMethods) {
    int registered = 0;
    for (int i = 0; i < numMethods; i++) {
        if ((*env)->RegisterNatives(env, clazz, &methods[i], 1) == 0) {
            registered++;
        } else {
            (*env)->ExceptionClear(env);
        }
    }
    return registered;
}

/* ==================== Float/Double bit conversion (ART intrinsics fallback) ==================== */
static jint Float_floatToRawIntBits(JNIEnv* env, jclass cls, jfloat f) {
    union { jfloat f; jint i; } u; u.f = f; return u.i;
}
static jfloat Float_intBitsToFloat(JNIEnv* env, jclass cls, jint i) {
    union { jfloat f; jint i; } u; u.i = i; return u.f;
}
static jlong Double_doubleToRawLongBits(JNIEnv* env, jclass cls, jdouble d) {
    union { jdouble d; jlong l; } u; u.d = d; return u.l;
}
static jdouble Double_longBitsToDouble(JNIEnv* env, jclass cls, jlong l) {
    union { jdouble d; jlong l; } u; u.l = l; return u.d;
}

/* Called by ART during early init — registers critical natives before class loading */
__attribute__((constructor)) void westlake_register_intrinsics() {
    /* These will be registered later via JNI_OnLoad, but we mark them available */
}

/* ==================== java.lang.System natives ==================== */

static jobjectArray System_specialProperties(JNIEnv* env, jclass ignored) {
    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    if (!stringClass) return NULL;
    jobjectArray result = (*env)->NewObjectArray(env, 4, stringClass, NULL);
    if (!result) return NULL;
    (*env)->DeleteLocalRef(env, stringClass);

    char path[PATH_MAX];
    char* cwd = getcwd(path, sizeof(path));
    if (!cwd) cwd = "/";
    char user_dir[PATH_MAX + 16];
    snprintf(user_dir, sizeof(user_dir), "user.dir=%s", cwd);
    (*env)->SetObjectArrayElement(env, result, 0, (*env)->NewStringUTF(env, user_dir));
    (*env)->SetObjectArrayElement(env, result, 1,
        (*env)->NewStringUTF(env, "android.zlib.version=1.2.11"));
    (*env)->SetObjectArrayElement(env, result, 2,
        (*env)->NewStringUTF(env, "android.openssl.version=OpenSSL 1.1.1k stub"));

    const char* lib_path = getenv("LD_LIBRARY_PATH");
    if (!lib_path) lib_path = "";
    char* java_path = (char*)malloc(strlen("java.library.path=") + strlen(lib_path) + 1);
    strcpy(java_path, "java.library.path=");
    strcat(java_path, lib_path);
    (*env)->SetObjectArrayElement(env, result, 3, (*env)->NewStringUTF(env, java_path));
    free(java_path);

    return result;
}

static jlong System_nanoTime(JNIEnv* env, jclass ignored) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (jlong)now.tv_sec * 1000000000LL + now.tv_nsec;
}

static jlong System_currentTimeMillis(JNIEnv* env, jclass ignored) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return (jlong)now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;
}

static jstring System_mapLibraryName(JNIEnv* env, jclass ignored, jstring libname) {
    if (!libname) return NULL;
    const char* name = (*env)->GetStringUTFChars(env, libname, NULL);
    char buf[512];
    snprintf(buf, sizeof(buf), "lib%s.so", name);
    (*env)->ReleaseStringUTFChars(env, libname, name);
    return (*env)->NewStringUTF(env, buf);
}

static void System_log(JNIEnv* env, jclass ignored, jchar type, jstring msg, jthrowable exc) {
    if (msg) {
        const char* s = (*env)->GetStringUTFChars(env, msg, NULL);
        fprintf(stderr, "System.log(%c): %s\n", (char)type, s);
        (*env)->ReleaseStringUTFChars(env, msg, s);
    }
}

static void System_setErr0(JNIEnv* env, jclass clazz, jobject stream) { }
static void System_setOut0(JNIEnv* env, jclass clazz, jobject stream) { }
static void System_setIn0(JNIEnv* env, jclass clazz, jobject stream) { }

/* ==================== sun.misc.Version natives ==================== */

static jstring Version_getJvmSpecialVersion(JNIEnv* env, jclass clazz) {
    return (*env)->NewStringUTF(env, "");
}
static jstring Version_getJdkSpecialVersion(JNIEnv* env, jclass clazz) {
    return (*env)->NewStringUTF(env, "");
}
static jboolean Version_getJvmVersionInfo(JNIEnv* env, jclass clazz) {
    return JNI_FALSE;
}
static void Version_getJdkVersionInfo(JNIEnv* env, jclass clazz) { }

/* ==================== java.io.FileDescriptor natives ==================== */

static jboolean FileDescriptor_isSocket(JNIEnv* env, jclass clazz, jint fd) {
    struct stat sb;
    if (fstat(fd, &sb) == 0) {
        return S_ISSOCK(sb.st_mode) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

static void FileDescriptor_sync(JNIEnv* env, jobject thiz) {
    jclass cls = (*env)->GetObjectClass(env, thiz);
    jfieldID descField = (*env)->GetFieldID(env, cls, "descriptor", "I");
    if (!descField) return;
    int fd = (*env)->GetIntField(env, thiz, descField);
    if (fd >= 0) {
        if (fsync(fd) < 0) {
            jclass ioExCls = (*env)->FindClass(env, "java/io/SyncFailedException");
            if (ioExCls) {
                (*env)->ThrowNew(env, ioExCls, strerror(errno));
            }
        }
    }
}

/* ==================== sun.nio.ch.FileDispatcherImpl natives ==================== */

static int getChannelFd(JNIEnv* env, jobject fdObj) {
    if (!fdObj) return -1;
    jclass fdCls = (*env)->GetObjectClass(env, fdObj);
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (!descField) return -1;
    return (*env)->GetIntField(env, fdObj, descField);
}

static void FileDispatcherImpl_init(JNIEnv* env, jclass clazz) {
    /* no-op */
}

static jint FileDispatcherImpl_read0(JNIEnv* env, jobject thiz, jobject fdObj, jlong address, jint len) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    ssize_t n = read(fd, (void*)(uintptr_t)address, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -2; /* IOS_UNAVAILABLE */
        jclass ioExCls = (*env)->FindClass(env, "java/io/IOException");
        if (ioExCls) (*env)->ThrowNew(env, ioExCls, strerror(errno));
        return -1;
    }
    return (n == 0) ? -1 : (jint)n; /* -1 = EOF */
}

static jint FileDispatcherImpl_write0(JNIEnv* env, jobject thiz, jobject fdObj, jlong address, jint len) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    ssize_t n = write(fd, (void*)(uintptr_t)address, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
        jclass ioExCls = (*env)->FindClass(env, "java/io/IOException");
        if (ioExCls) (*env)->ThrowNew(env, ioExCls, strerror(errno));
        return -1;
    }
    return (jint)n;
}

static jint FileDispatcherImpl_pread0(JNIEnv* env, jobject thiz, jobject fdObj, jlong address, jint len, jlong offset) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    ssize_t n = pread(fd, (void*)(uintptr_t)address, len, (off_t)offset);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
        jclass ioExCls = (*env)->FindClass(env, "java/io/IOException");
        if (ioExCls) (*env)->ThrowNew(env, ioExCls, strerror(errno));
        return -1;
    }
    return (n == 0) ? -1 : (jint)n;
}

static jint FileDispatcherImpl_pwrite0(JNIEnv* env, jobject thiz, jobject fdObj, jlong address, jint len, jlong offset) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    ssize_t n = pwrite(fd, (void*)(uintptr_t)address, len, (off_t)offset);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
        jclass ioExCls = (*env)->FindClass(env, "java/io/IOException");
        if (ioExCls) (*env)->ThrowNew(env, ioExCls, strerror(errno));
        return -1;
    }
    return (jint)n;
}

static jlong FileDispatcherImpl_size0(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    struct stat sb;
    if (fstat(fd, &sb) < 0) return -1;
    return (jlong)sb.st_size;
}

static void FileDispatcherImpl_close0(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getChannelFd(env, fdObj);
    if (fd >= 0) close(fd);
}

static jint FileDispatcherImpl_force0(JNIEnv* env, jobject thiz, jobject fdObj, jboolean metadata) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    int rc = metadata ? fsync(fd) : fdatasync(fd);
    if (rc < 0) {
        jclass ioExCls = (*env)->FindClass(env, "java/io/IOException");
        if (ioExCls) (*env)->ThrowNew(env, ioExCls, strerror(errno));
        return -1;
    }
    return 0;
}

static jint FileDispatcherImpl_truncate0(JNIEnv* env, jobject thiz, jobject fdObj, jlong size) {
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    if (ftruncate(fd, (off_t)size) < 0) {
        jclass ioExCls = (*env)->FindClass(env, "java/io/IOException");
        if (ioExCls) (*env)->ThrowNew(env, ioExCls, strerror(errno));
        return -1;
    }
    return 0;
}

/* ==================== java.io.FileOutputStream natives ==================== */

static void FileOutputStream_initIDs(JNIEnv* env, jclass clazz) { /* no-op */ }

static void FileOutputStream_open0(JNIEnv* env, jobject thiz, jstring jname, jboolean append) {
    if (!jname) return;
    const char* name = (*env)->GetStringUTFChars(env, jname, NULL);
    int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
    int fd = open(name, flags, 0666);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    if (fd < 0) {
        jclass fnfCls = (*env)->FindClass(env, "java/io/FileNotFoundException");
        if (fnfCls) (*env)->ThrowNew(env, fnfCls, strerror(errno));
        return;
    }
    /* Set fd on the FileDescriptor field */
    jclass fosCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fosCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) { close(fd); return; }
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    if (!fdObj) { close(fd); return; }
    jclass fdCls = (*env)->GetObjectClass(env, fdObj);
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (descField) (*env)->SetIntField(env, fdObj, descField, fd);
}

static void FileOutputStream_write(JNIEnv* env, jobject thiz, jint b, jboolean append) {
    jclass fosCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fosCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) return;
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return;
    char c = (char)b;
    write(fd, &c, 1);
}

static void FileOutputStream_writeBytes(JNIEnv* env, jobject thiz,
                                          jbyteArray bytes, jint off, jint len, jboolean append) {
    jclass fosCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fosCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) return;
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return;
    jbyte* buf = (*env)->GetByteArrayElements(env, bytes, NULL);
    if (!buf) return;
    write(fd, buf + off, len);
    (*env)->ReleaseByteArrayElements(env, bytes, buf, JNI_ABORT);
}

static void FileOutputStream_close0(JNIEnv* env, jobject thiz) {
    /* Closing handled by IoBridge/Os on ART -- no-op here */
}

/* ==================== java.io.FileInputStream natives ==================== */

static void FileInputStream_initIDs(JNIEnv* env, jclass clazz) { /* no-op */ }

static void FileInputStream_open0(JNIEnv* env, jobject thiz, jstring jname) {
    if (!jname) return;
    const char* name = (*env)->GetStringUTFChars(env, jname, NULL);
    int fd = open(name, O_RDONLY);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    if (fd < 0) {
        jclass fnfCls = (*env)->FindClass(env, "java/io/FileNotFoundException");
        if (fnfCls) (*env)->ThrowNew(env, fnfCls, strerror(errno));
        return;
    }
    jclass fisCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fisCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) { close(fd); return; }
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    if (!fdObj) { close(fd); return; }
    jclass fdCls = (*env)->GetObjectClass(env, fdObj);
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (descField) (*env)->SetIntField(env, fdObj, descField, fd);
}

static jint FileInputStream_read0(JNIEnv* env, jobject thiz) {
    jclass fisCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fisCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) return -1;
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    unsigned char c;
    ssize_t n = read(fd, &c, 1);
    return (n <= 0) ? -1 : (jint)c;
}

static jint FileInputStream_readBytes(JNIEnv* env, jobject thiz,
                                       jbyteArray bytes, jint off, jint len) {
    jclass fisCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fisCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) return -1;
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return -1;
    jbyte* buf = (*env)->GetByteArrayElements(env, bytes, NULL);
    if (!buf) return -1;
    ssize_t n = read(fd, buf + off, len);
    (*env)->ReleaseByteArrayElements(env, bytes, buf, 0);
    return (n <= 0) ? -1 : (jint)n;
}

static jlong FileInputStream_skip0(JNIEnv* env, jobject thiz, jlong n) {
    jclass fisCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fisCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) return 0;
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return 0;
    off_t cur = lseek(fd, 0, SEEK_CUR);
    if (cur == (off_t)-1) return 0;
    off_t end = lseek(fd, (off_t)n, SEEK_CUR);
    if (end == (off_t)-1) return 0;
    return (jlong)(end - cur);
}

static jint FileInputStream_available0(JNIEnv* env, jobject thiz) {
    jclass fisCls = (*env)->GetObjectClass(env, thiz);
    jfieldID fdField = (*env)->GetFieldID(env, fisCls, "fd", "Ljava/io/FileDescriptor;");
    if (!fdField) return 0;
    jobject fdObj = (*env)->GetObjectField(env, thiz, fdField);
    int fd = getChannelFd(env, fdObj);
    if (fd < 0) return 0;
    struct stat sb;
    if (fstat(fd, &sb) < 0) return 0;
    off_t cur = lseek(fd, 0, SEEK_CUR);
    if (cur == (off_t)-1) return 0;
    jint avail = (jint)(sb.st_size - cur);
    return (avail > 0) ? avail : 0;
}

static void FileInputStream_close0(JNIEnv* env, jobject thiz) {
    /* Closing handled by IoBridge/Os on ART -- no-op here */
}

/* ==================== java.io.UnixFileSystem natives ==================== */

/* Constants from java.io.FileSystem */
#define BA_EXISTS    0x01
#define BA_REGULAR   0x02
#define BA_DIRECTORY 0x04

static void UnixFileSystem_initIDs(JNIEnv* env, jclass clazz) { /* no-op */ }

static jint UnixFileSystem_getBooleanAttributes0(JNIEnv* env, jobject thiz, jstring jpath) {
    if (!jpath) return 0;
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    struct stat sb;
    int rv = 0;
    if (stat(path, &sb) == 0) {
        int fmt = sb.st_mode & S_IFMT;
        rv = BA_EXISTS
            | ((fmt == S_IFREG) ? BA_REGULAR : 0)
            | ((fmt == S_IFDIR) ? BA_DIRECTORY : 0);
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    return rv;
}

static jstring UnixFileSystem_canonicalize0(JNIEnv* env, jobject thiz, jstring jpath) {
    if (!jpath) return NULL;
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    char resolved[PATH_MAX];
    char* result = realpath(path, resolved);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (result) {
        return (*env)->NewStringUTF(env, resolved);
    }
    /* If realpath fails, return the original path */
    return jpath;
}

static jlong UnixFileSystem_getLastModifiedTime0(JNIEnv* env, jobject thiz, jobject file) {
    jclass fileCls = (*env)->GetObjectClass(env, file);
    jmethodID getPath = (*env)->GetMethodID(env, fileCls, "getPath", "()Ljava/lang/String;");
    jstring jpath = (jstring)(*env)->CallObjectMethod(env, file, getPath);
    if (!jpath) return 0;
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    struct stat sb;
    jlong result = 0;
    if (stat(path, &sb) == 0) {
        result = (jlong)sb.st_mtime * 1000LL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    return result;
}

static jboolean UnixFileSystem_createFileExclusively0(JNIEnv* env, jobject thiz, jstring jpath) {
    if (!jpath) return JNI_FALSE;
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0666);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (fd >= 0) {
        close(fd);
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

static jboolean UnixFileSystem_createDirectory0(JNIEnv* env, jobject thiz, jobject file) {
    jclass fileCls = (*env)->GetObjectClass(env, file);
    jmethodID getPath = (*env)->GetMethodID(env, fileCls, "getPath", "()Ljava/lang/String;");
    jstring jpath = (jstring)(*env)->CallObjectMethod(env, file, getPath);
    if (!jpath) return JNI_FALSE;
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    int result = mkdir(path, 0777);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    return result == 0 ? JNI_TRUE : JNI_FALSE;
}

static jobjectArray UnixFileSystem_list0(JNIEnv* env, jobject thiz, jobject file) {
    jclass fileCls = (*env)->GetObjectClass(env, file);
    jmethodID getPath = (*env)->GetMethodID(env, fileCls, "getPath", "()Ljava/lang/String;");
    jstring jpath = (jstring)(*env)->CallObjectMethod(env, file, getPath);
    if (!jpath) return NULL;
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    DIR* dir = opendir(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (!dir) return NULL;

    /* Count entries first */
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            count++;
    }
    rewinddir(dir);

    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray(env, count, stringCls, NULL);
    int i = 0;
    while ((entry = readdir(dir)) != NULL && i < count) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            (*env)->SetObjectArrayElement(env, result, i++, (*env)->NewStringUTF(env, entry->d_name));
        }
    }
    closedir(dir);
    (*env)->DeleteLocalRef(env, stringCls);
    return result;
}

static jboolean UnixFileSystem_setPermission0(JNIEnv* env, jobject thiz, jobject file,
                                                jint access, jboolean enable, jboolean owneronly) {
    return JNI_FALSE; /* stub */
}

static jboolean UnixFileSystem_setLastModifiedTime0(JNIEnv* env, jobject thiz, jobject file, jlong time) {
    return JNI_FALSE; /* stub */
}

static jboolean UnixFileSystem_setReadOnly0(JNIEnv* env, jobject thiz, jobject file) {
    return JNI_FALSE; /* stub */
}

static jstring UnixFileSystem_parentOrNull(JNIEnv* env, jobject thiz, jstring jpath) {
    return NULL; /* ART will use Java fallback */
}

static jlong UnixFileSystem_getSpace0(JNIEnv* env, jobject thiz, jobject file, jint t) {
    return 0; /* stub */
}

/* ==================== java.lang.Runtime natives ==================== */

static void Runtime_nativeExit(JNIEnv* env, jclass clazz, jint status) {
    _exit(status);
}

static void Runtime_nativeGc(JNIEnv* env, jobject thiz) {
    /* no-op stub */
}

static jstring Runtime_nativeLoad(JNIEnv* env, jclass clazz, jstring filename,
                                   jobject classLoader, jclass caller) {
    if (!filename) return (*env)->NewStringUTF(env, "null filename");
    const char* path = (*env)->GetStringUTFChars(env, filename, NULL);
    /* Return success for statically-linked libraries */
    if (strstr(path, "javacore") || strstr(path, "openjdk") ||
        strstr(path, "icu_jni") || strstr(path, "icu-jni")) {
        (*env)->ReleaseStringUTFChars(env, filename, path);
        return NULL; /* null = success, already registered */
    }
    /* OHBridge: register methods now (deferred from InitNativeMethods) */
    if (strstr(path, "oh_bridge")) {
        (*env)->ReleaseStringUTFChars(env, filename, path);
        JavaVM* vm; (*env)->GetJavaVM(env, &vm);
        JNI_OnLoad_ohbridge(vm, NULL);
        return NULL; /* null = success */
    }
    /* Try dlopen for other libraries */
    void* handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        const char* err = dlerror();
        jstring result = (*env)->NewStringUTF(env, err ? err : "dlopen not supported");
        (*env)->ReleaseStringUTFChars(env, filename, path);
        return result;
    }
    /* Call JNI_OnLoad if present */
    typedef jint (*JNI_OnLoad_fn)(JavaVM*, void*);
    JNI_OnLoad_fn onLoad = (JNI_OnLoad_fn)dlsym(handle, "JNI_OnLoad");
    if (onLoad) {
        JavaVM* vm;
        (*env)->GetJavaVM(env, &vm);
        jint ver = onLoad(vm, NULL);
        if (ver < 0) {
            dlclose(handle);
            (*env)->ReleaseStringUTFChars(env, filename, path);
            return (*env)->NewStringUTF(env, "JNI_OnLoad returned error");
        }
    }
    (*env)->ReleaseStringUTFChars(env, filename, path);
    return NULL; /* null = success */
}

static jlong Runtime_freeMemory(JNIEnv* env, jobject thiz) {
    return 64 * 1024 * 1024; /* 64 MB */
}

static jlong Runtime_totalMemory(JNIEnv* env, jobject thiz) {
    return 256 * 1024 * 1024; /* 256 MB */
}

static jlong Runtime_maxMemory(JNIEnv* env, jobject thiz) {
    return 512 * 1024 * 1024; /* 512 MB */
}

/* java.lang.Runtime.runFinalization0 */
static void Runtime_runFinalization0(JNIEnv* env, jclass clazz) {
    /* no-op stub */
}

/* ==================== JNI_OnLoad ==================== */
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>

/* Minimal ZIP reading for java.util.zip.ZipFile */
/* We implement the subset ART's classloader needs */

/* ZIP file handle - just stores the fd and mmap */
typedef struct {
    int fd;
    uint8_t *data;
    size_t size;
    /* Central directory */
    uint8_t *cd_start;
    uint32_t cd_entries;
} NativeZipFile;

/* Find EOCD (End of Central Directory) */
static uint8_t* find_eocd(uint8_t* data, size_t size) {
    if (size < 22) return NULL;
    /* Search backwards from end */
    for (size_t i = size - 22; i > 0 && i > size - 65557; i--) {
        if (data[i] == 0x50 && data[i+1] == 0x4b && data[i+2] == 0x05 && data[i+3] == 0x06)
            return &data[i];
    }
    return NULL;
}

/* open(String name, int mode, long lastModified, boolean usemmap) -> long */
static jlong ZipFile_open(JNIEnv* env, jclass cls, jstring jname, jint mode, jlong lastMod, jboolean usemmap) {
    if (!jname) return 0;
    const char* name = (*env)->GetStringUTFChars(env, jname, NULL);
    int fd = open(name, O_RDONLY);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    if (fd < 0) return 0;
    
    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return 0; }
    
    uint8_t* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { close(fd); return 0; }
    
    NativeZipFile* zf = calloc(1, sizeof(NativeZipFile));
    zf->fd = fd;
    zf->data = data;
    zf->size = st.st_size;
    
    /* Parse EOCD */
    uint8_t* eocd = find_eocd(data, st.st_size);
    if (eocd) {
        zf->cd_entries = eocd[8] | (eocd[9] << 8);
        uint32_t cd_offset = eocd[16] | (eocd[17] << 8) | (eocd[18] << 16) | (eocd[19] << 24);
        if (cd_offset < st.st_size) zf->cd_start = data + cd_offset;
    }
    
    return (jlong)(uintptr_t)zf;
}

/* close(long jzfile) */
static void ZipFile_close(JNIEnv* env, jclass cls, jlong handle) {
    NativeZipFile* zf = (NativeZipFile*)(uintptr_t)handle;
    if (!zf) return;
    if (zf->data) munmap(zf->data, zf->size);
    if (zf->fd >= 0) close(zf->fd);
    free(zf);
}

/* getTotal(long jzfile) -> int */
static jint ZipFile_getTotal(JNIEnv* env, jclass cls, jlong handle) {
    NativeZipFile* zf = (NativeZipFile*)(uintptr_t)handle;
    return zf ? (jint)zf->cd_entries : 0;
}

/* getEntry(long jzfile, byte[] name, boolean addSlash) -> long */
static jlong ZipFile_getEntry(JNIEnv* env, jclass cls, jlong handle, jbyteArray jname, jboolean addSlash) {
    NativeZipFile* zf = (NativeZipFile*)(uintptr_t)handle;
    if (!zf || !zf->cd_start || !jname) return 0;
    
    jsize nameLen = (*env)->GetArrayLength(env, jname);
    jbyte* nameBytes = (*env)->GetByteArrayElements(env, jname, NULL);
    
    /* Search central directory for matching entry */
    uint8_t* p = zf->cd_start;
    uint8_t* end = zf->data + zf->size;
    for (uint32_t i = 0; i < zf->cd_entries && p + 46 <= end; i++) {
        if (p[0] != 0x50 || p[1] != 0x4b || p[2] != 0x01 || p[3] != 0x02) break;
        uint16_t fnLen = p[28] | (p[29] << 8);
        uint16_t extraLen = p[30] | (p[31] << 8);
        uint16_t commentLen = p[32] | (p[33] << 8);
        
        if (fnLen == nameLen && p + 46 + fnLen <= end &&
            memcmp(p + 46, nameBytes, nameLen) == 0) {
            (*env)->ReleaseByteArrayElements(env, jname, nameBytes, JNI_ABORT);
            return (jlong)(uintptr_t)p; /* return pointer to CD entry as handle */
        }
        p += 46 + fnLen + extraLen + commentLen;
    }
    (*env)->ReleaseByteArrayElements(env, jname, nameBytes, JNI_ABORT);
    return 0;
}

/* getEntryBytes(long jzentry, int type) -> byte[] */
static jbyteArray ZipFile_getEntryBytes(JNIEnv* env, jclass cls, jlong entry, jint type) {
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!p) return NULL;
    /* type: 0=name, 1=extra, 2=comment */
    uint16_t fnLen = p[28] | (p[29] << 8);
    if (type == 0) {
        jbyteArray result = (*env)->NewByteArray(env, fnLen);
        (*env)->SetByteArrayRegion(env, result, 0, fnLen, (jbyte*)(p + 46));
        return result;
    }
    return NULL;
}

/* getEntrySize(long jzentry) -> long (uncompressed size) */
static jlong ZipFile_getEntrySize(JNIEnv* env, jclass cls, jlong entry) {
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!p) return -1;
    return p[24] | (p[25] << 8) | (p[26] << 16) | (p[27] << 24);
}

/* getEntryCSize(long jzentry) -> long (compressed size) */
static jlong ZipFile_getEntryCSize(JNIEnv* env, jclass cls, jlong entry) {
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!p) return -1;
    return p[20] | (p[21] << 8) | (p[22] << 16) | (p[23] << 24);
}

/* getEntryMethod(long jzentry) -> int */
static jint ZipFile_getEntryMethod(JNIEnv* env, jclass cls, jlong entry) {
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!p) return 0;
    return p[10] | (p[11] << 8);
}

/* getEntryTime(long jzentry) -> long */
static jlong ZipFile_getEntryTime(JNIEnv* env, jclass cls, jlong entry) {
    return 0; /* stub */
}

/* getEntryCrc(long jzentry) -> long */
static jlong ZipFile_getEntryCrc(JNIEnv* env, jclass cls, jlong entry) {
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!p) return 0;
    return p[16] | (p[17] << 8) | (p[18] << 16) | (p[19] << 24);
}

/* getEntryFlag(long jzentry) -> int */
static jint ZipFile_getEntryFlag(JNIEnv* env, jclass cls, jlong entry) {
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!p) return 0;
    return p[8] | (p[9] << 8);
}

/* getFileDescriptor(long jzfile) -> int */
static jint ZipFile_getFileDescriptor(JNIEnv* env, jclass cls, jlong handle) {
    NativeZipFile* zf = (NativeZipFile*)(uintptr_t)handle;
    return zf ? zf->fd : -1;
}

/* getCommentBytes(long jzfile) -> byte[] */
static jbyteArray ZipFile_getCommentBytes(JNIEnv* env, jclass cls, jlong handle) {
    return NULL;
}

/* read(long jzfile, long jzentry, long pos, byte[] b, int off, int len) -> int */
static jint ZipFile_read(JNIEnv* env, jclass cls, jlong handle, jlong entry, jlong pos,
                          jbyteArray buf, jint off, jint len) {
    NativeZipFile* zf = (NativeZipFile*)(uintptr_t)handle;
    uint8_t* p = (uint8_t*)(uintptr_t)entry;
    if (!zf || !p || !buf) return -1;
    
    /* Get local file header offset from CD entry */
    uint32_t localOff = p[42] | (p[43] << 8) | (p[44] << 16) | (p[45] << 24);
    if (localOff + 30 >= zf->size) return -1;
    
    /* Parse local file header */
    uint8_t* lh = zf->data + localOff;
    uint16_t lfnLen = lh[26] | (lh[27] << 8);
    uint16_t lextraLen = lh[28] | (lh[29] << 8);
    uint8_t* fileData = lh + 30 + lfnLen + lextraLen;
    
    uint32_t csize = p[20] | (p[21] << 8) | (p[22] << 16) | (p[23] << 24);
    if (pos >= csize) return -1;
    
    jint toRead = len;
    if (pos + toRead > csize) toRead = (jint)(csize - pos);
    
    (*env)->SetByteArrayRegion(env, buf, off, toRead, (jbyte*)(fileData + pos));
    return toRead;
}

/* Stubs for less critical methods */
static void ZipFile_ensureOpen(JNIEnv* env, jobject thiz) { }
static jobject ZipFile_getInflater(JNIEnv* env, jobject thiz) { return NULL; }
static void ZipFile_releaseInflater(JNIEnv* env, jobject thiz, jobject inflater) { }
static jobject ZipFile_getZipEntry(JNIEnv* env, jobject thiz, jstring name, jlong entry) { return NULL; }

/* startsWithLOC - check if ZIP starts with local file header */
static void ZipFile_freeEntry(JNIEnv* env, jclass cls, jlong handle, jlong entry) { /* no-op: entries are in mmap'd zip */ }

static jboolean ZipFile_startsWithLOC(JNIEnv* env, jclass cls, jlong handle) {
    NativeZipFile* zf = (NativeZipFile*)(uintptr_t)handle;
    if (!zf || zf->size < 4) return JNI_FALSE;
    return (zf->data[0] == 0x50 && zf->data[1] == 0x4b &&
            zf->data[2] == 0x03 && zf->data[3] == 0x04) ? JNI_TRUE : JNI_FALSE;
}
#include <math.h>

/* java.lang.Math native methods */
static jdouble Math_sin(JNIEnv* e, jclass c, jdouble a) { return sin(a); }
static jdouble Math_cos(JNIEnv* e, jclass c, jdouble a) { return cos(a); }
static jdouble Math_tan(JNIEnv* e, jclass c, jdouble a) { return tan(a); }
static jdouble Math_asin(JNIEnv* e, jclass c, jdouble a) { return asin(a); }
static jdouble Math_acos(JNIEnv* e, jclass c, jdouble a) { return acos(a); }
static jdouble Math_atan(JNIEnv* e, jclass c, jdouble a) { return atan(a); }
static jdouble Math_atan2(JNIEnv* e, jclass c, jdouble a, jdouble b) { return atan2(a, b); }
static jdouble Math_exp(JNIEnv* e, jclass c, jdouble a) { return exp(a); }
static jdouble Math_log(JNIEnv* e, jclass c, jdouble a) { return log(a); }
static jdouble Math_log10(JNIEnv* e, jclass c, jdouble a) { return log10(a); }
static jdouble Math_sqrt(JNIEnv* e, jclass c, jdouble a) { return sqrt(a); }
static jdouble Math_cbrt(JNIEnv* e, jclass c, jdouble a) { return cbrt(a); }
static jdouble Math_ceil(JNIEnv* e, jclass c, jdouble a) { return ceil(a); }
static jdouble Math_floor(JNIEnv* e, jclass c, jdouble a) { return floor(a); }
static jdouble Math_pow(JNIEnv* e, jclass c, jdouble a, jdouble b) { return pow(a, b); }
static jdouble Math_sinh(JNIEnv* e, jclass c, jdouble a) { return sinh(a); }
static jdouble Math_cosh(JNIEnv* e, jclass c, jdouble a) { return cosh(a); }
static jdouble Math_tanh(JNIEnv* e, jclass c, jdouble a) { return tanh(a); }
static jdouble Math_expm1(JNIEnv* e, jclass c, jdouble a) { return expm1(a); }
static jdouble Math_log1p(JNIEnv* e, jclass c, jdouble a) { return log1p(a); }
static jdouble Math_IEEEremainder(JNIEnv* e, jclass c, jdouble a, jdouble b) { return remainder(a, b); }
static jdouble Math_hypot(JNIEnv* e, jclass c, jdouble a, jdouble b) { return hypot(a, b); }
static jdouble Math_abs_d(JNIEnv* e, jclass c, jdouble a) { return fabs(a); }
static jdouble Math_max_d(JNIEnv* e, jclass c, jdouble a, jdouble b) { return fmax(a, b); }
static jdouble Math_min_d(JNIEnv* e, jclass c, jdouble a, jdouble b) { return fmin(a, b); }
static jdouble Math_copySign_d(JNIEnv* e, jclass c, jdouble a, jdouble b) { return copysign(a, b); }
static jdouble Math_toDegrees(JNIEnv* e, jclass c, jdouble a) { return a * (180.0 / 3.14159265358979323846); }
static jdouble Math_toRadians(JNIEnv* e, jclass c, jdouble a) { return a * (3.14159265358979323846 / 180.0); }
static jdouble Math_random(JNIEnv* e, jclass c) { return (double)rand() / RAND_MAX; }
static jdouble Math_rint(JNIEnv* e, jclass c, jdouble a) { return rint(a); }
static jint Math_round_f(JNIEnv* e, jclass c, jfloat a) { return (jint)roundf(a); }
static jlong Math_round_d(JNIEnv* e, jclass c, jdouble a) { return (jlong)round(a); }

/* ==================== java.lang.Character natives ==================== */
#include <ctype.h>
#include <wctype.h>

static jboolean Character_isDigitImpl(JNIEnv*e,jclass c,jint cp) { return (cp>='0'&&cp<='9')?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isUpperCaseImpl(JNIEnv*e,jclass c,jint cp) { return (cp>='A'&&cp<='Z')?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isLowerCaseImpl(JNIEnv*e,jclass c,jint cp) { return (cp>='a'&&cp<='z')?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isLetterImpl(JNIEnv*e,jclass c,jint cp) { return ((cp>='A'&&cp<='Z')||(cp>='a'&&cp<='z'))?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isLetterOrDigitImpl(JNIEnv*e,jclass c,jint cp) { return ((cp>='A'&&cp<='Z')||(cp>='a'&&cp<='z')||(cp>='0'&&cp<='9'))?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isAlphabeticImpl(JNIEnv*e,jclass c,jint cp) { return ((cp>='A'&&cp<='Z')||(cp>='a'&&cp<='z'))?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isDefinedImpl(JNIEnv*e,jclass c,jint cp) { return (cp>=0x20&&cp<=0x7e)?JNI_TRUE:JNI_FALSE; }
static jint Character_toTitleCaseImpl(JNIEnv*e,jclass c,jint cp) { return (cp>='a'&&cp<='z')?(cp-32):cp; }
static jboolean Character_isIdentifierIgnorableImpl(JNIEnv*e,jclass c,jint cp) { return JNI_FALSE; }
static jint Character_getNumericValueImpl(JNIEnv*e,jclass c,jint cp) { if(cp>='0'&&cp<='9') return cp-'0'; if(cp>='A'&&cp<='Z') return cp-'A'+10; if(cp>='a'&&cp<='z') return cp-'a'+10; return -1; }
static jbyte Character_getDirectionalityImpl(JNIEnv*e,jclass c,jint cp) { return 0; }
static jboolean Character_isMirroredImpl(JNIEnv*e,jclass c,jint cp) { return JNI_FALSE; }
static jboolean Character_isTitleCaseImpl(JNIEnv*e,jclass c,jint cp) { return JNI_FALSE; }
static jboolean Character_isIdeographicImpl(JNIEnv*e,jclass c,jint cp) { return JNI_FALSE; }
static jint Character_toUpperCaseImpl(JNIEnv*e,jclass c,jint cp) { return (cp>='a'&&cp<='z')?(cp-32):cp; }
static jint Character_toLowerCaseImpl(JNIEnv*e,jclass c,jint cp) { return (cp>='A'&&cp<='Z')?(cp+32):cp; }
static jboolean Character_isWhitespaceImpl(JNIEnv*e,jclass c,jint cp) { return (cp==' '||cp=='\t'||cp=='\n'||cp=='\r'||cp=='\f')?JNI_TRUE:JNI_FALSE; }
static jboolean Character_isSpaceCharImpl(JNIEnv*e,jclass c,jint cp) { return (cp==' '||cp==0xA0)?JNI_TRUE:JNI_FALSE; }
static jint Character_digitImpl(JNIEnv*e,jclass c,jint cp,jint radix) {
    if(radix<2||radix>36) return -1;
    int v=-1;
    if(cp>='0'&&cp<='9') v=cp-'0';
    else if(cp>='A'&&cp<='Z') v=cp-'A'+10;
    else if(cp>='a'&&cp<='z') v=cp-'a'+10;
    return (v>=0&&v<radix)?v:-1;
}
static jint Character_getTypeImpl(JNIEnv*e,jclass c,jint cp) {
    if(cp>='A'&&cp<='Z') return 1; /* UPPERCASE_LETTER */
    if(cp>='a'&&cp<='z') return 2; /* LOWERCASE_LETTER */
    if(cp>='0'&&cp<='9') return 9; /* DECIMAL_DIGIT_NUMBER */
    if(cp==' '||cp=='\t'||cp=='\n') return 15; /* SPACE_SEPARATOR approx */
    return 0; /* UNASSIGNED */
}

/* ==================== libcore.util.NativeAllocationRegistry ==================== */
static void NativeAllocationRegistry_applyFreeFunction(JNIEnv*e,jclass c,jlong freeFunc,jlong ptr) {
    /* Call the free function pointer if non-null */
    if (freeFunc != 0 && ptr != 0) {
        typedef void (*FreeFn)(void*);
        FreeFn fn = (FreeFn)(intptr_t)freeFunc;
        fn((void*)(intptr_t)ptr);
    }
}

/* ==================== android.graphics.Typeface stubs ==================== */
static jlong Typeface_nativeGetReleaseFunc(JNIEnv* env, jclass cls) { return 0; }
static jlong Typeface_nativeCreateFromTypeface(JNIEnv* env, jclass cls, jlong ni, jint style) { return ni ? ni : 1; }
static jlong Typeface_nativeCreateFromTypefaceWithExactStyle(JNIEnv* env, jclass cls, jlong ni, jint weight, jboolean italic) { return ni ? ni : 1; }
static jlong Typeface_nativeCreateWeightAlias(JNIEnv* env, jclass cls, jlong ni, jint weight) { return ni ? ni : 1; }
static jlong Typeface_nativeCreateFromArray(JNIEnv* env, jclass cls, jlongArray familyArray, jint weight, jint italic) { return 1; }
static jintArray Typeface_nativeGetSupportedAxes(JNIEnv* env, jclass cls, jlong ni) { return NULL; }
static void Typeface_nativeSetDefault(JNIEnv* env, jclass cls, jlong nativePtr) {}
static jint Typeface_nativeGetStyle(JNIEnv* env, jclass cls, jlong nativePtr) { return 0; /* NORMAL */ }
static jint Typeface_nativeGetWeight(JNIEnv* env, jclass cls, jlong nativePtr) { return 400; /* regular */ }
static void Typeface_nativeRegisterGenericFamily(JNIEnv* env, jclass cls, jstring str, jlong nativePtr) {}

/* ==================== java.util.zip.Inflater ==================== */
/* Wrapper to track input buffer alongside z_stream (can't use opaque — zlib uses it) */
typedef struct {
    z_stream strm;
    Bytef* input_buf;  /* allocated input buffer to free later */
} InflaterState;

static jlong Inflater_init(JNIEnv* env, jclass cls, jboolean nowrap) {
    InflaterState* state = (InflaterState*)calloc(1, sizeof(InflaterState));
    if (!state) return 0;
    int ret = inflateInit2(&state->strm, nowrap ? -MAX_WBITS : MAX_WBITS);
    if (ret != Z_OK) { free(state); return 0; }
    return (jlong)(intptr_t)state;
}
static void Inflater_setDictionary(JNIEnv* env, jclass cls, jlong addr, jbyteArray b, jint off, jint len) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    if (!state) return;
    jbyte* buf = (*env)->GetByteArrayElements(env, b, NULL);
    inflateSetDictionary(&state->strm, (Bytef*)(buf + off), len);
    (*env)->ReleaseByteArrayElements(env, b, buf, JNI_ABORT);
}
static void Inflater_setInput(JNIEnv* env, jclass cls, jlong addr, jbyteArray b, jint off, jint len) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    if (!state || !b) return;
    if (state->input_buf) { free(state->input_buf); state->input_buf = NULL; }
    jbyte* src = (*env)->GetByteArrayElements(env, b, NULL);
    Bytef* buf = (Bytef*)malloc(len);
    memcpy(buf, src + off, len);
    (*env)->ReleaseByteArrayElements(env, b, src, JNI_ABORT);
    state->strm.next_in = buf;
    state->strm.avail_in = len;
    state->input_buf = buf;
}
static jint Inflater_inflateBytes(JNIEnv* env, jobject obj, jlong addr, jbyteArray b, jint off, jint len) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    if (!state) return -1;
    z_stream* strm = &state->strm;
    if (strm->avail_in == 0) {
        /* No input — if we've already decompressed some data, signal finished */
        if (strm->total_in > 0) {
            jclass infCls = (*env)->GetObjectClass(env, obj);
            jfieldID fid = (*env)->GetFieldID(env, infCls, "finished", "Z");
            if (fid) (*env)->SetBooleanField(env, obj, fid, JNI_TRUE);
        }
        return 0;
    }
    jbyte* buf = (*env)->GetByteArrayElements(env, b, NULL);
    strm->next_out = (Bytef*)(buf + off);
    strm->avail_out = len;
    int ret = inflate(strm, Z_NO_FLUSH);
    int n = len - strm->avail_out;
    (*env)->ReleaseByteArrayElements(env, b, buf, 0);
    if (ret == Z_STREAM_END) {
        jclass infCls = (*env)->GetObjectClass(env, obj);
        jfieldID fid = (*env)->GetFieldID(env, infCls, "finished", "Z");
        if (fid) (*env)->SetBooleanField(env, obj, fid, JNI_TRUE);
    } else if (ret == Z_NEED_DICT) {
        jclass infCls = (*env)->GetObjectClass(env, obj);
        jfieldID fid = (*env)->GetFieldID(env, infCls, "needDict", "Z");
        if (fid) (*env)->SetBooleanField(env, obj, fid, JNI_TRUE);
    }
    return n;
}
static jint Inflater_getAdler(JNIEnv* env, jclass cls, jlong addr) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    return state ? (jint)state->strm.adler : 0;
}
static void Inflater_reset(JNIEnv* env, jclass cls, jlong addr) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    if (state) inflateReset(&state->strm);
}
static jlong Inflater_getBytesRead(JNIEnv* env, jclass cls, jlong addr) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    return state ? (jlong)state->strm.total_in : 0;
}
static jlong Inflater_getBytesWritten(JNIEnv* env, jclass cls, jlong addr) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    return state ? (jlong)state->strm.total_out : 0;
}
static void Inflater_end(JNIEnv* env, jclass cls, jlong addr) {
    InflaterState* state = (InflaterState*)(intptr_t)addr;
    if (state) {
        if (state->input_buf) free(state->input_buf);
        inflateEnd(&state->strm);
        free(state);
    }
}

/* ==================== java.util.zip.Deflater ==================== */
static jlong Deflater_init(JNIEnv* env, jclass cls, jint level, jint strategy, jboolean nowrap) {
    z_stream* strm = (z_stream*)calloc(1, sizeof(z_stream));
    if (!strm) return 0;
    int ret = deflateInit2(strm, level, Z_DEFLATED, nowrap ? -MAX_WBITS : MAX_WBITS, 8, strategy);
    if (ret != Z_OK) { free(strm); return 0; }
    return (jlong)(intptr_t)strm;
}
static void Deflater_setDictionary(JNIEnv* env, jclass cls, jlong addr, jbyteArray b, jint off, jint len) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    if (!strm) return;
    jbyte* buf = (*env)->GetByteArrayElements(env, b, NULL);
    deflateSetDictionary(strm, (Bytef*)(buf + off), len);
    (*env)->ReleaseByteArrayElements(env, b, buf, JNI_ABORT);
}
static void Deflater_setInput(JNIEnv* env, jclass cls, jlong addr, jbyteArray b, jint off, jint len) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    if (!strm || !b) return;
    if (strm->opaque) { free(strm->opaque); strm->opaque = NULL; }
    jbyte* src = (*env)->GetByteArrayElements(env, b, NULL);
    Bytef* buf = (Bytef*)malloc(len);
    memcpy(buf, src + off, len);
    (*env)->ReleaseByteArrayElements(env, b, src, JNI_ABORT);
    strm->next_in = buf;
    strm->avail_in = len;
    strm->opaque = buf;
}
static jint Deflater_deflateBytes(JNIEnv* env, jobject obj, jlong addr, jbyteArray b, jint off, jint len, jint flush) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    if (!strm) return 0;
    jbyte* buf = (*env)->GetByteArrayElements(env, b, NULL);
    strm->next_out = (Bytef*)(buf + off);
    strm->avail_out = len;
    int ret = deflate(strm, flush);
    int n = len - strm->avail_out;
    (*env)->ReleaseByteArrayElements(env, b, buf, 0);
    if (ret == Z_STREAM_END) {
        jclass defCls = (*env)->GetObjectClass(env, obj);
        jfieldID fid = (*env)->GetFieldID(env, defCls, "finished", "Z");
        if (fid) (*env)->SetBooleanField(env, obj, fid, JNI_TRUE);
    }
    return n;
}
static jint Deflater_getAdler(JNIEnv* env, jclass cls, jlong addr) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    return strm ? (jint)strm->adler : 0;
}
static void Deflater_reset(JNIEnv* env, jclass cls, jlong addr) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    if (strm) deflateReset(strm);
}
static jlong Deflater_getBytesRead(JNIEnv* env, jclass cls, jlong addr) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    return strm ? (jlong)strm->total_in : 0;
}
static jlong Deflater_getBytesWritten(JNIEnv* env, jclass cls, jlong addr) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    return strm ? (jlong)strm->total_out : 0;
}
static void Deflater_end(JNIEnv* env, jclass cls, jlong addr) {
    z_stream* strm = (z_stream*)(intptr_t)addr;
    if (strm) {
        if (strm->opaque) free(strm->opaque);
        deflateEnd(strm);
        free(strm);
    }
}

/* ==================== com.android.icu.util.regex.PatternNative ==================== */
/* Wraps POSIX regex for basic java.util.regex support */
typedef struct {
    regex_t preg;
    int compiled;
    char* pattern_str;
} IcuPattern;

static jlong PatternNative_getNativeFinalizer(JNIEnv* env, jclass cls) {
    /* Return a function pointer that frees an IcuPattern */
    return (jlong)(intptr_t)free; /* simplified: just free the struct */
}

static jlong PatternNative_compileRegex(JNIEnv* env, jclass cls, jstring pattern, jint flags) {
    IcuPattern* pat = (IcuPattern*)calloc(1, sizeof(IcuPattern));
    if (!pat) return 0;
    const char* str = (*env)->GetStringUTFChars(env, pattern, NULL);
    pat->pattern_str = strdup(str);
    int cflags = REG_EXTENDED;
    if (flags & 2) cflags |= REG_ICASE; /* CASE_INSENSITIVE */
    if (flags & 32) cflags |= REG_NEWLINE; /* MULTILINE */
    int rc = regcomp(&pat->preg, str, cflags);
    pat->compiled = (rc == 0) ? 1 : 0;
    (*env)->ReleaseStringUTFChars(env, pattern, str);
    return (jlong)(intptr_t)pat;
}

static jlong PatternNative_openMatcher(JNIEnv* env, jclass cls, jlong patAddr) {
    /* Return the pattern address as matcher handle — matcher state is per-call */
    return patAddr ? patAddr : 1;
}

/* ==================== com.android.icu.util.regex.MatcherNative ==================== */
static jboolean MatcherNative_findNext(JNIEnv* env, jclass cls, jlong matcherAddr, jint startIndex) {
    return JNI_FALSE; /* stub: no match */
}

static jboolean MatcherNative_find(JNIEnv* env, jclass cls, jlong matcherAddr) {
    return JNI_FALSE;
}

static jboolean MatcherNative_lookingAt(JNIEnv* env, jclass cls, jlong matcherAddr) {
    return JNI_FALSE;
}

static jboolean MatcherNative_matches(JNIEnv* env, jclass cls, jlong matcherAddr) {
    return JNI_FALSE;
}

static jint MatcherNative_groupCount(JNIEnv* env, jclass cls, jlong matcherAddr) {
    return 0;
}

static jint MatcherNative_start(JNIEnv* env, jclass cls, jlong matcherAddr, jint group) {
    return -1;
}

static jint MatcherNative_end(JNIEnv* env, jclass cls, jlong matcherAddr, jint group) {
    return -1;
}

static void MatcherNative_setInput(JNIEnv* env, jclass cls, jlong matcherAddr, jstring input, jint start, jint end) {}
static void MatcherNative_useAnchoringBounds(JNIEnv* env, jclass cls, jlong matcherAddr, jboolean b) {}
static void MatcherNative_useTransparentBounds(JNIEnv* env, jclass cls, jlong matcherAddr, jboolean b) {}
static jint MatcherNative_hitEnd(JNIEnv* env, jclass cls, jlong matcherAddr) { return 0; }
static jint MatcherNative_requireEnd(JNIEnv* env, jclass cls, jlong matcherAddr) { return 0; }
static jlong MatcherNative_getNativeFinalizer(JNIEnv* env, jclass cls) { return (jlong)(intptr_t)free; }

/* ==================== java.util.jar.JarFile stubs ==================== */
static jobjectArray JarFile_getMetaInfEntryNames(JNIEnv* env, jobject obj) {
    /* Return null — no META-INF entries, skips jar verification */
    return NULL;
}

/* ==================== java.security.SecureRandom fallback ==================== */
/* For UUID.randomUUID() — seed from /dev/urandom if available, else time-based */
static jbyteArray SecureRandom_engineNextBytes_stub = NULL;

/* Forward declare ohbridge JNI_OnLoad - we call it ourselves since the weak link may not */
extern jint JNI_OnLoad_ohbridge(JavaVM* vm, void* reserved);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) return -1;

    /* java.lang.Float — must be first (needed by Math.<clinit>) */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/Float");
        if (cls) {
            JNINativeMethod methods[] = {
                {"floatToRawIntBits", "(F)I", (void*)Float_floatToRawIntBits},
                {"intBitsToFloat", "(I)F", (void*)Float_intBitsToFloat},
            };
            registerNativesOrSkip(env, cls, methods, 2);
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.lang.Double */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/Double");
        if (cls) {
            JNINativeMethod methods[] = {
                {"doubleToRawLongBits", "(D)J", (void*)Double_doubleToRawLongBits},
                {"longBitsToDouble", "(J)D", (void*)Double_longBitsToDouble},
            };
            registerNativesOrSkip(env, cls, methods, 2);
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.lang.System */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/System");
        if (cls) {
            JNINativeMethod methods[] = {
                {"specialProperties", "()[Ljava/lang/String;", (void*)System_specialProperties},
                {"mapLibraryName", "(Ljava/lang/String;)Ljava/lang/String;", (void*)System_mapLibraryName},
                {"setErr0", "(Ljava/io/PrintStream;)V", (void*)System_setErr0},
                {"setOut0", "(Ljava/io/PrintStream;)V", (void*)System_setOut0},
                {"setIn0", "(Ljava/io/InputStream;)V", (void*)System_setIn0},
                {"log", "(CLjava/lang/String;Ljava/lang/Throwable;)V", (void*)System_log},
                {"nanoTime", "()J", (void*)System_nanoTime},
                {"currentTimeMillis", "()J", (void*)System_currentTimeMillis},
            };
            registerNativesOrSkip(env, cls, methods, 8);
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* sun.misc.Version */
    {
        jclass cls = (*env)->FindClass(env, "sun/misc/Version");
        if (cls) {
            JNINativeMethod methods[] = {
                {"getJvmSpecialVersion", "()Ljava/lang/String;", (void*)Version_getJvmSpecialVersion},
                {"getJdkSpecialVersion", "()Ljava/lang/String;", (void*)Version_getJdkSpecialVersion},
                {"getJvmVersionInfo", "()Z", (void*)Version_getJvmVersionInfo},
                {"getJdkVersionInfo", "()V", (void*)Version_getJdkVersionInfo},
            };
            registerNativesOrSkip(env, cls, methods, 4);
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.io.FileDescriptor */
    {
        jclass cls = (*env)->FindClass(env, "java/io/FileDescriptor");
        if (cls) {
            JNINativeMethod methods[] = {
                {"isSocket", "(I)Z", (void*)FileDescriptor_isSocket},
                {"sync", "()V", (void*)FileDescriptor_sync},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* sun.nio.ch.FileDispatcherImpl */
    {
        jclass cls = (*env)->FindClass(env, "sun/nio/ch/FileDispatcherImpl");
        if (cls) {
            JNINativeMethod methods[] = {
                {"read0", "(Ljava/io/FileDescriptor;JI)I", (void*)FileDispatcherImpl_read0},
                {"write0", "(Ljava/io/FileDescriptor;JI)I", (void*)FileDispatcherImpl_write0},
                {"pread0", "(Ljava/io/FileDescriptor;JIJ)I", (void*)FileDispatcherImpl_pread0},
                {"pwrite0", "(Ljava/io/FileDescriptor;JIJ)I", (void*)FileDispatcherImpl_pwrite0},
                {"size0", "(Ljava/io/FileDescriptor;)J", (void*)FileDispatcherImpl_size0},
                {"close0", "(Ljava/io/FileDescriptor;)V", (void*)FileDispatcherImpl_close0},
                {"force0", "(Ljava/io/FileDescriptor;Z)I", (void*)FileDispatcherImpl_force0},
                {"truncate0", "(Ljava/io/FileDescriptor;J)I", (void*)FileDispatcherImpl_truncate0},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.io.FileOutputStream */
    {
        jclass cls = (*env)->FindClass(env, "java/io/FileOutputStream");
        if (cls) {
            JNINativeMethod methods[] = {
                {"open0", "(Ljava/lang/String;Z)V", (void*)FileOutputStream_open0},
                {"write", "(IZ)V", (void*)FileOutputStream_write},
                {"writeBytes", "([BIIZ)V", (void*)FileOutputStream_writeBytes},
                {"close0", "()V", (void*)FileOutputStream_close0},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.io.FileInputStream */
    {
        jclass cls = (*env)->FindClass(env, "java/io/FileInputStream");
        if (cls) {
            JNINativeMethod methods[] = {
                {"open0", "(Ljava/lang/String;)V", (void*)FileInputStream_open0},
                {"read0", "()I", (void*)FileInputStream_read0},
                {"readBytes", "([BII)I", (void*)FileInputStream_readBytes},
                {"skip0", "(J)J", (void*)FileInputStream_skip0},
                {"available0", "()I", (void*)FileInputStream_available0},
                {"close0", "()V", (void*)FileInputStream_close0},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.lang.Character */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/Character");
        if (cls) {
            JNINativeMethod methods[] = {
                {"isDigitImpl", "(I)Z", (void*)Character_isDigitImpl},
                {"isUpperCaseImpl", "(I)Z", (void*)Character_isUpperCaseImpl},
                {"isLowerCaseImpl", "(I)Z", (void*)Character_isLowerCaseImpl},
                {"isLetterImpl", "(I)Z", (void*)Character_isLetterImpl},
                {"toUpperCaseImpl", "(I)I", (void*)Character_toUpperCaseImpl},
                {"toLowerCaseImpl", "(I)I", (void*)Character_toLowerCaseImpl},
                {"isWhitespaceImpl", "(I)Z", (void*)Character_isWhitespaceImpl},
                {"isSpaceCharImpl", "(I)Z", (void*)Character_isSpaceCharImpl},
                {"digitImpl", "(II)I", (void*)Character_digitImpl},
                {"getTypeImpl", "(I)I", (void*)Character_getTypeImpl},
                {"isLetterOrDigitImpl", "(I)Z", (void*)Character_isLetterOrDigitImpl},
                {"isAlphabeticImpl", "(I)Z", (void*)Character_isAlphabeticImpl},
                {"isDefinedImpl", "(I)Z", (void*)Character_isDefinedImpl},
                {"toTitleCaseImpl", "(I)I", (void*)Character_toTitleCaseImpl},
                {"isIdentifierIgnorableImpl", "(I)Z", (void*)Character_isIdentifierIgnorableImpl},
                {"getNumericValueImpl", "(I)I", (void*)Character_getNumericValueImpl},
                {"getDirectionalityImpl", "(I)B", (void*)Character_getDirectionalityImpl},
                {"isMirroredImpl", "(I)Z", (void*)Character_isMirroredImpl},
                {"isTitleCaseImpl", "(I)Z", (void*)Character_isTitleCaseImpl},
                {"isIdeographicImpl", "(I)Z", (void*)Character_isIdeographicImpl},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* libcore.util.NativeAllocationRegistry */
    {
        jclass cls = (*env)->FindClass(env, "libcore/util/NativeAllocationRegistry");
        if (cls) {
            JNINativeMethod methods[] = {
                {"applyFreeFunction", "(JJ)V", (void*)NativeAllocationRegistry_applyFreeFunction},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.lang.Runtime */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/Runtime");
        if (cls) {
            JNINativeMethod methods[] = {
                {"nativeExit", "(I)V", (void*)Runtime_nativeExit},
                {"nativeGc", "()V", (void*)Runtime_nativeGc},
                {"nativeLoad", "(Ljava/lang/String;Ljava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/String;", (void*)Runtime_nativeLoad},
                {"freeMemory", "()J", (void*)Runtime_freeMemory},
                {"totalMemory", "()J", (void*)Runtime_totalMemory},
                {"maxMemory", "()J", (void*)Runtime_maxMemory},
                {"runFinalization0", "()V", (void*)Runtime_runFinalization0},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.io.UnixFileSystem */
    {
        jclass cls = (*env)->FindClass(env, "java/io/UnixFileSystem");
        if (cls) {
            JNINativeMethod methods[] = {
                {"initIDs", "()V", (void*)UnixFileSystem_initIDs},
                {"getBooleanAttributes0", "(Ljava/lang/String;)I", (void*)UnixFileSystem_getBooleanAttributes0},
                {"canonicalize0", "(Ljava/lang/String;)Ljava/lang/String;", (void*)UnixFileSystem_canonicalize0},
                {"canonicalize", "(Ljava/lang/String;)Ljava/lang/String;", (void*)UnixFileSystem_canonicalize0},
                {"parentOrNull", "(Ljava/lang/String;)Ljava/lang/String;", (void*)UnixFileSystem_parentOrNull},
                {"getLastModifiedTime0", "(Ljava/io/File;)J", (void*)UnixFileSystem_getLastModifiedTime0},
                {"createFileExclusively0", "(Ljava/lang/String;)Z", (void*)UnixFileSystem_createFileExclusively0},
                {"createDirectory0", "(Ljava/io/File;)Z", (void*)UnixFileSystem_createDirectory0},
                {"list0", "(Ljava/io/File;)[Ljava/lang/String;", (void*)UnixFileSystem_list0},
                {"setPermission0", "(Ljava/io/File;IZZ)Z", (void*)UnixFileSystem_setPermission0},
                {"setLastModifiedTime0", "(Ljava/io/File;J)Z", (void*)UnixFileSystem_setLastModifiedTime0},
                {"setReadOnly0", "(Ljava/io/File;)Z", (void*)UnixFileSystem_setReadOnly0},
                {"getSpace0", "(Ljava/io/File;I)J", (void*)UnixFileSystem_getSpace0},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.util.zip.ZipFile */
    {
        jclass cls = (*env)->FindClass(env, "java/util/zip/ZipFile");
        if (cls) {
            JNINativeMethod methods[] = {
                {"open", "(Ljava/lang/String;IJZ)J", (void*)ZipFile_open},
                {"close", "(J)V", (void*)ZipFile_close},
                {"getTotal", "(J)I", (void*)ZipFile_getTotal},
                {"getEntry", "(J[BZ)J", (void*)ZipFile_getEntry},
                {"getEntryBytes", "(JI)[B", (void*)ZipFile_getEntryBytes},
                {"getEntrySize", "(J)J", (void*)ZipFile_getEntrySize},
                {"getEntryCSize", "(J)J", (void*)ZipFile_getEntryCSize},
                {"getEntryMethod", "(J)I", (void*)ZipFile_getEntryMethod},
                {"getEntryTime", "(J)J", (void*)ZipFile_getEntryTime},
                {"getEntryCrc", "(J)J", (void*)ZipFile_getEntryCrc},
                {"getEntryFlag", "(J)I", (void*)ZipFile_getEntryFlag},
                {"getFileDescriptor", "(J)I", (void*)ZipFile_getFileDescriptor},
                {"getCommentBytes", "(J)[B", (void*)ZipFile_getCommentBytes},
                {"read", "(JJJ[BII)I", (void*)ZipFile_read},
                {"freeEntry", "(JJ)V", (void*)ZipFile_freeEntry},
                {"startsWithLOC", "(J)Z", (void*)ZipFile_startsWithLOC},
                {"ensureOpen", "()V", (void*)ZipFile_ensureOpen},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* java.util.jar.JarFile */
    {
        jclass cls = (*env)->FindClass(env, "java/util/jar/JarFile");
        if (cls) {
            JNINativeMethod methods[] = {
                {"getMetaInfEntryNames", "()[Ljava/lang/String;", (void*)JarFile_getMetaInfEntryNames},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* java.util.zip.Inflater */
    {
        jclass cls = (*env)->FindClass(env, "java/util/zip/Inflater");
        if (cls) {
            JNINativeMethod methods[] = {
                {"init", "(Z)J", (void*)Inflater_init},
                {"setInput", "(J[BII)V", (void*)Inflater_setInput},
                {"setDictionary", "(J[BII)V", (void*)Inflater_setDictionary},
                {"inflateBytes", "(J[BII)I", (void*)Inflater_inflateBytes},
                {"getAdler", "(J)I", (void*)Inflater_getAdler},
                {"getBytesRead", "(J)J", (void*)Inflater_getBytesRead},
                {"getBytesWritten", "(J)J", (void*)Inflater_getBytesWritten},
                {"reset", "(J)V", (void*)Inflater_reset},
                {"end", "(J)V", (void*)Inflater_end},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* java.util.zip.Deflater */
    {
        jclass cls = (*env)->FindClass(env, "java/util/zip/Deflater");
        if (cls) {
            JNINativeMethod methods[] = {
                {"init", "(IIZ)J", (void*)Deflater_init},
                {"setInput", "(J[BII)V", (void*)Deflater_setInput},
                {"setDictionary", "(J[BII)V", (void*)Deflater_setDictionary},
                {"deflateBytes", "(J[BII)I", (void*)Deflater_deflateBytes},
                {"getAdler", "(J)I", (void*)Deflater_getAdler},
                {"getBytesRead", "(J)J", (void*)Deflater_getBytesRead},
                {"getBytesWritten", "(J)J", (void*)Deflater_getBytesWritten},
                {"reset", "(J)V", (void*)Deflater_reset},
                {"end", "(J)V", (void*)Deflater_end},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* com.android.icu.util.regex.PatternNative */
    {
        jclass cls = (*env)->FindClass(env, "com/android/icu/util/regex/PatternNative");
        if (cls) {
            JNINativeMethod methods[] = {
                {"getNativeFinalizer", "()J", (void*)PatternNative_getNativeFinalizer},
                {"compileRegex", "(Ljava/lang/String;I)J", (void*)PatternNative_compileRegex},
                {"compileImpl", "(Ljava/lang/String;I)J", (void*)PatternNative_compileRegex},
                {"openMatcher", "(J)J", (void*)PatternNative_openMatcher},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* com.android.icu.util.regex.MatcherNative */
    {
        jclass cls = (*env)->FindClass(env, "com/android/icu/util/regex/MatcherNative");
        if (cls) {
            JNINativeMethod methods[] = {
                {"getNativeFinalizer", "()J", (void*)MatcherNative_getNativeFinalizer},
                {"find", "(J[II)Z", (void*)MatcherNative_findNext},
                {"findNext", "(JI[I)Z", (void*)MatcherNative_findNext},
                {"lookingAt", "(J[I)Z", (void*)MatcherNative_lookingAt},
                {"matches", "(J[I)Z", (void*)MatcherNative_matches},
                {"groupCount", "(J)I", (void*)MatcherNative_groupCount},
                {"setInput", "(JLjava/lang/String;II)V", (void*)MatcherNative_setInput},
                {"useAnchoringBounds", "(JZ)V", (void*)MatcherNative_useAnchoringBounds},
                {"useTransparentBounds", "(JZ)V", (void*)MatcherNative_useTransparentBounds},
                {"hitEnd", "(J)Z", (void*)MatcherNative_hitEnd},
                {"requireEnd", "(J)Z", (void*)MatcherNative_requireEnd},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* java.lang.Math */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/Math");
        if (cls) {
            JNINativeMethod methods[] = {
                {"sin","(D)D",(void*)Math_sin},{"cos","(D)D",(void*)Math_cos},
                {"tan","(D)D",(void*)Math_tan},{"asin","(D)D",(void*)Math_asin},
                {"acos","(D)D",(void*)Math_acos},{"atan","(D)D",(void*)Math_atan},
                {"atan2","(DD)D",(void*)Math_atan2},{"exp","(D)D",(void*)Math_exp},
                {"log","(D)D",(void*)Math_log},{"log10","(D)D",(void*)Math_log10},
                {"sqrt","(D)D",(void*)Math_sqrt},{"cbrt","(D)D",(void*)Math_cbrt},
                {"ceil","(D)D",(void*)Math_ceil},{"floor","(D)D",(void*)Math_floor},
                {"pow","(DD)D",(void*)Math_pow},{"sinh","(D)D",(void*)Math_sinh},
                {"cosh","(D)D",(void*)Math_cosh},{"tanh","(D)D",(void*)Math_tanh},
                {"expm1","(D)D",(void*)Math_expm1},{"log1p","(D)D",(void*)Math_log1p},
                {"IEEEremainder","(DD)D",(void*)Math_IEEEremainder},
                {"hypot","(DD)D",(void*)Math_hypot},
                {"abs","(D)D",(void*)Math_abs_d},{"max","(DD)D",(void*)Math_max_d},
                {"copySign","(DD)D",(void*)Math_copySign_d},
                {"toDegrees","(D)D",(void*)Math_toDegrees},
                {"round","(F)I",(void*)Math_round_f},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* java.lang.StrictMath */
    {
        jclass cls = (*env)->FindClass(env, "java/lang/StrictMath");
        if (cls) {
            JNINativeMethod methods[] = {
                {"sin","(D)D",(void*)Math_sin},{"cos","(D)D",(void*)Math_cos},
                {"tan","(D)D",(void*)Math_tan},{"asin","(D)D",(void*)Math_asin},
                {"acos","(D)D",(void*)Math_acos},{"atan","(D)D",(void*)Math_atan},
                {"atan2","(DD)D",(void*)Math_atan2},{"exp","(D)D",(void*)Math_exp},
                {"log","(D)D",(void*)Math_log},{"log10","(D)D",(void*)Math_log10},
                {"sqrt","(D)D",(void*)Math_sqrt},{"cbrt","(D)D",(void*)Math_cbrt},
                {"ceil","(D)D",(void*)Math_ceil},{"floor","(D)D",(void*)Math_floor},
                {"pow","(DD)D",(void*)Math_pow},{"sinh","(D)D",(void*)Math_sinh},
                {"cosh","(D)D",(void*)Math_cosh},{"tanh","(D)D",(void*)Math_tanh},
                {"expm1","(D)D",(void*)Math_expm1},{"log1p","(D)D",(void*)Math_log1p},
                {"abs","(D)D",(void*)Math_abs_d},{"max","(DD)D",(void*)Math_max_d},
                {"toDegrees","(D)D",(void*)Math_toDegrees},
                {"random","()D",(void*)Math_random},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }
    /* android.graphics.Typeface — needed before any View rendering */
    {
        jclass cls = (*env)->FindClass(env, "android/graphics/Typeface");
        if (cls) {
            JNINativeMethod methods[] = {
                {"nativeGetReleaseFunc", "()J", (void*)Typeface_nativeGetReleaseFunc},
                {"nativeCreateFromTypeface", "(JI)J", (void*)Typeface_nativeCreateFromTypeface},
                {"nativeCreateFromTypefaceWithExactStyle", "(JIZ)J", (void*)Typeface_nativeCreateFromTypefaceWithExactStyle},
                {"nativeCreateWeightAlias", "(JI)J", (void*)Typeface_nativeCreateWeightAlias},
                {"nativeCreateFromArray", "([JII)J", (void*)Typeface_nativeCreateFromArray},
                {"nativeGetSupportedAxes", "(J)[I", (void*)Typeface_nativeGetSupportedAxes},
                {"nativeSetDefault", "(J)V", (void*)Typeface_nativeSetDefault},
                {"nativeGetStyle", "(J)I", (void*)Typeface_nativeGetStyle},
                {"nativeGetWeight", "(J)I", (void*)Typeface_nativeGetWeight},
                {"nativeRegisterGenericFamily", "(Ljava/lang/String;J)V", (void*)Typeface_nativeRegisterGenericFamily},
            };
            registerNativesOrSkip(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* OHBridge registered later via Runtime_nativeLoad when System.loadLibrary is called */

    /* Clear any pending exceptions from failed RegisterNatives calls
       (speed boot image compiles some methods as intrinsics, making them non-native) */
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
    }

    return JNI_VERSION_1_6;
}
