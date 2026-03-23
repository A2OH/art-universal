#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

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
    /* Return null = success, non-null = error message */
    return NULL;
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

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) return -1;

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
            (*env)->RegisterNatives(env, cls, methods, 8);
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
            (*env)->RegisterNatives(env, cls, methods, 4);
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
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* sun.nio.ch.FileDispatcherImpl */
    {
        jclass cls = (*env)->FindClass(env, "sun/nio/ch/FileDispatcherImpl");
        if (cls) {
            JNINativeMethod methods[] = {
                {"init", "()V", (void*)FileDispatcherImpl_init},
                {"read0", "(Ljava/io/FileDescriptor;JI)I", (void*)FileDispatcherImpl_read0},
                {"write0", "(Ljava/io/FileDescriptor;JI)I", (void*)FileDispatcherImpl_write0},
                {"pread0", "(Ljava/io/FileDescriptor;JIJ)I", (void*)FileDispatcherImpl_pread0},
                {"pwrite0", "(Ljava/io/FileDescriptor;JIJ)I", (void*)FileDispatcherImpl_pwrite0},
                {"size0", "(Ljava/io/FileDescriptor;)J", (void*)FileDispatcherImpl_size0},
                {"close0", "(Ljava/io/FileDescriptor;)V", (void*)FileDispatcherImpl_close0},
                {"force0", "(Ljava/io/FileDescriptor;Z)I", (void*)FileDispatcherImpl_force0},
                {"truncate0", "(Ljava/io/FileDescriptor;J)I", (void*)FileDispatcherImpl_truncate0},
            };
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.io.FileOutputStream */
    {
        jclass cls = (*env)->FindClass(env, "java/io/FileOutputStream");
        if (cls) {
            JNINativeMethod methods[] = {
                {"initIDs", "()V", (void*)FileOutputStream_initIDs},
                {"open0", "(Ljava/lang/String;Z)V", (void*)FileOutputStream_open0},
                {"write", "(IZ)V", (void*)FileOutputStream_write},
                {"writeBytes", "([BIIZ)V", (void*)FileOutputStream_writeBytes},
                {"close0", "()V", (void*)FileOutputStream_close0},
            };
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    /* java.io.FileInputStream */
    {
        jclass cls = (*env)->FindClass(env, "java/io/FileInputStream");
        if (cls) {
            JNINativeMethod methods[] = {
                {"initIDs", "()V", (void*)FileInputStream_initIDs},
                {"open0", "(Ljava/lang/String;)V", (void*)FileInputStream_open0},
                {"read0", "()I", (void*)FileInputStream_read0},
                {"readBytes", "([BII)I", (void*)FileInputStream_readBytes},
                {"skip0", "(J)J", (void*)FileInputStream_skip0},
                {"available0", "()I", (void*)FileInputStream_available0},
                {"close0", "()V", (void*)FileInputStream_close0},
            };
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
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
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
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
                {"getLastModifiedTime0", "(Ljava/io/File;)J", (void*)UnixFileSystem_getLastModifiedTime0},
                {"createFileExclusively0", "(Ljava/lang/String;)Z", (void*)UnixFileSystem_createFileExclusively0},
                {"createDirectory0", "(Ljava/io/File;)Z", (void*)UnixFileSystem_createDirectory0},
                {"list0", "(Ljava/io/File;)[Ljava/lang/String;", (void*)UnixFileSystem_list0},
                {"setPermission0", "(Ljava/io/File;IZZ)Z", (void*)UnixFileSystem_setPermission0},
                {"setLastModifiedTime0", "(Ljava/io/File;J)Z", (void*)UnixFileSystem_setLastModifiedTime0},
                {"setReadOnly0", "(Ljava/io/File;)Z", (void*)UnixFileSystem_setReadOnly0},
                {"getSpace0", "(Ljava/io/File;I)J", (void*)UnixFileSystem_getSpace0},
            };
            (*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    return JNI_VERSION_1_6;
}
