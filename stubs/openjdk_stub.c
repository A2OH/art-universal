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
    return JNI_FALSE;
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
            };
            (*env)->RegisterNatives(env, cls, methods, 1);
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
