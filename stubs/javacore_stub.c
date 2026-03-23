#include <jni.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>

/* Stub for registerNativesOrSkip - just calls RegisterNatives, ignoring failures */
static int registerNativesOrSkip(JNIEnv* env, jclass clazz,
                                  const JNINativeMethod* methods, int numMethods) {
    int registered = 0; for (int i = 0; i < numMethods; i++) { if ((*env)->RegisterNatives(env, clazz, &methods[i], 1) == 0) registered++; else (*env)->ExceptionClear(env); } return registered;
    if (result != 0) {
        (*env)->ExceptionClear(env);
    }
    return result;
}

/* ==================== Helper: get fd int from FileDescriptor object ==================== */

static int getFd(JNIEnv* env, jobject fdObj) {
    if (!fdObj) return -1;
    jclass fdCls = (*env)->GetObjectClass(env, fdObj);
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (!descField) return -1;
    return (*env)->GetIntField(env, fdObj, descField);
}

static void throwErrnoException(JNIEnv* env, const char* functionName, int errnum) {
    jclass cls = (*env)->FindClass(env, "android/system/ErrnoException");
    if (!cls) return;
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/lang/String;I)V");
    if (!ctor) return;
    jstring name = (*env)->NewStringUTF(env, functionName);
    jobject exc = (*env)->NewObject(env, cls, ctor, name, (jint)errnum);
    if (exc) (*env)->Throw(env, (jthrowable)exc);
}

/* ==================== libcore.io.Linux native methods ==================== */

/* getpwuid */
static jobject linux_getpwuid(JNIEnv* env, jobject thiz, jint uid) {
    struct passwd* pw = getpwuid(uid);
    const char* pw_name = pw ? pw->pw_name : "user";
    int pw_uid = pw ? pw->pw_uid : uid;
    int pw_gid = pw ? pw->pw_gid : uid;
    const char* pw_dir = pw ? pw->pw_dir : "/";
    const char* pw_shell = pw ? pw->pw_shell : "/bin/sh";

    jclass cls = (*env)->FindClass(env, "android/system/StructPasswd");
    if (!cls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;)V");
    if (!ctor) return NULL;
    return (*env)->NewObject(env, cls, ctor,
        (*env)->NewStringUTF(env, pw_name),
        (jint)pw_uid, (jint)pw_gid,
        (*env)->NewStringUTF(env, pw_dir),
        (*env)->NewStringUTF(env, pw_shell));
}

/* uname */
static jobject linux_uname(JNIEnv* env, jobject thiz) {
    struct utsname buf;
    uname(&buf);
    jclass cls = (*env)->FindClass(env, "android/system/StructUtsname");
    if (!cls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (!ctor) return NULL;
    return (*env)->NewObject(env, cls, ctor,
        (*env)->NewStringUTF(env, buf.sysname),
        (*env)->NewStringUTF(env, buf.nodename),
        (*env)->NewStringUTF(env, buf.release),
        (*env)->NewStringUTF(env, buf.version),
        (*env)->NewStringUTF(env, buf.machine));
}

/* getenv */
static jstring linux_getenv(JNIEnv* env, jobject thiz, jstring name) {
    const char* n = (*env)->GetStringUTFChars(env, name, NULL);
    const char* val = getenv(n);
    (*env)->ReleaseStringUTFChars(env, name, n);
    return val ? (*env)->NewStringUTF(env, val) : NULL;
}

/* getuid / getpid / geteuid / getegid / getgid / getppid */
static jint linux_getuid(JNIEnv* env, jobject thiz) { return getuid(); }
static jint linux_getpid(JNIEnv* env, jobject thiz) { return getpid(); }
static jint linux_geteuid(JNIEnv* env, jobject thiz) { return geteuid(); }
static jint linux_getegid(JNIEnv* env, jobject thiz) { return getegid(); }
static jint linux_getgid(JNIEnv* env, jobject thiz) { return getgid(); }
static jint linux_getppid(JNIEnv* env, jobject thiz) { return getppid(); }

/* sysconf */
static jlong linux_sysconf(JNIEnv* env, jobject thiz, jint name) {
    long result = sysconf((int)name);
    return (jlong)result;
}

/* isatty */
static jboolean linux_isatty(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getFd(env, fdObj);
    return isatty(fd) ? JNI_TRUE : JNI_FALSE;
}

/* writeBytes(FileDescriptor fd, Object buffer, int offset, int byteCount) */
static jint linux_writeBytes(JNIEnv* env, jobject thiz,
                              jobject fdObj, jobject buffer, jint offset, jint byteCount) {
    int fd = getFd(env, fdObj);
    if (fd < 0) {
        throwErrnoException(env, "write", EBADF);
        return -1;
    }
    jbyteArray byteArray = (jbyteArray)buffer;
    jbyte* bytes = (*env)->GetByteArrayElements(env, byteArray, NULL);
    if (!bytes) return -1;
    ssize_t n = write(fd, bytes + offset, byteCount);
    (*env)->ReleaseByteArrayElements(env, byteArray, bytes, JNI_ABORT);
    if (n < 0) {
        throwErrnoException(env, "write", errno);
        return -1;
    }
    return (jint)n;
}

/* readBytes(FileDescriptor fd, Object buffer, int offset, int byteCount) */
static jint linux_readBytes(JNIEnv* env, jobject thiz,
                             jobject fdObj, jobject buffer, jint offset, jint byteCount) {
    int fd = getFd(env, fdObj);
    if (fd < 0) {
        throwErrnoException(env, "read", EBADF);
        return -1;
    }
    jbyteArray byteArray = (jbyteArray)buffer;
    jbyte* bytes = (*env)->GetByteArrayElements(env, byteArray, NULL);
    if (!bytes) return -1;
    ssize_t n = read(fd, bytes + offset, byteCount);
    (*env)->ReleaseByteArrayElements(env, byteArray, bytes, 0);
    if (n < 0) {
        throwErrnoException(env, "read", errno);
        return -1;
    }
    return (jint)n;
}

/* close(FileDescriptor fd) */
static void linux_close(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getFd(env, fdObj);
    if (fd >= 0) {
        close(fd);
    }
}

/* open(String path, int flags, int mode) -> FileDescriptor */
static jobject linux_open(JNIEnv* env, jobject thiz, jstring jpath, jint flags, jint mode) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    int fd = open(path, flags, mode);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (fd < 0) {
        throwErrnoException(env, "open", errno);
        return NULL;
    }
    jclass fdCls = (*env)->FindClass(env, "java/io/FileDescriptor");
    if (!fdCls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, fdCls, "<init>", "()V");
    if (!ctor) return NULL;
    jobject fdObj = (*env)->NewObject(env, fdCls, ctor);
    if (!fdObj) return NULL;
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (!descField) return NULL;
    (*env)->SetIntField(env, fdObj, descField, fd);
    return fdObj;
}

/* lseek(FileDescriptor fd, long offset, int whence) */
static jlong linux_lseek(JNIEnv* env, jobject thiz, jobject fdObj, jlong offset, jint whence) {
    int fd = getFd(env, fdObj);
    off_t result = lseek(fd, (off_t)offset, whence);
    if (result == -1) {
        throwErrnoException(env, "lseek", errno);
        return -1;
    }
    return (jlong)result;
}

/* fstat(FileDescriptor fd) -> StructStat */
static jobject linux_fstat(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getFd(env, fdObj);
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        throwErrnoException(env, "fstat", errno);
        return NULL;
    }
    jclass cls = (*env)->FindClass(env, "android/system/StructStat");
    if (!cls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "(JJIJIIJJJJJJJ)V");
    if (!ctor) return NULL;
    return (*env)->NewObject(env, cls, ctor,
        (jlong)sb.st_dev, (jlong)sb.st_ino, (jint)sb.st_mode, (jlong)sb.st_nlink,
        (jint)sb.st_uid, (jint)sb.st_gid, (jlong)sb.st_rdev, (jlong)sb.st_size,
        (jlong)sb.st_atime, (jlong)sb.st_mtime, (jlong)sb.st_ctime,
        (jlong)sb.st_blksize, (jlong)sb.st_blocks);
}

/* stat(String path) -> StructStat */
static jobject linux_stat(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    struct stat sb;
    int result = stat(path, &sb);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (result < 0) {
        throwErrnoException(env, "stat", errno);
        return NULL;
    }
    jclass cls = (*env)->FindClass(env, "android/system/StructStat");
    if (!cls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "(JJIJIIJJJJJJJ)V");
    if (!ctor) return NULL;
    return (*env)->NewObject(env, cls, ctor,
        (jlong)sb.st_dev, (jlong)sb.st_ino, (jint)sb.st_mode, (jlong)sb.st_nlink,
        (jint)sb.st_uid, (jint)sb.st_gid, (jlong)sb.st_rdev, (jlong)sb.st_size,
        (jlong)sb.st_atime, (jlong)sb.st_mtime, (jlong)sb.st_ctime,
        (jlong)sb.st_blksize, (jlong)sb.st_blocks);
}

/* lstat(String path) -> StructStat */
static jobject linux_lstat(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    struct stat sb;
    int result = lstat(path, &sb);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (result < 0) {
        throwErrnoException(env, "lstat", errno);
        return NULL;
    }
    jclass cls = (*env)->FindClass(env, "android/system/StructStat");
    if (!cls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "(JJIJIIJJJJJJJ)V");
    if (!ctor) return NULL;
    return (*env)->NewObject(env, cls, ctor,
        (jlong)sb.st_dev, (jlong)sb.st_ino, (jint)sb.st_mode, (jlong)sb.st_nlink,
        (jint)sb.st_uid, (jint)sb.st_gid, (jlong)sb.st_rdev, (jlong)sb.st_size,
        (jlong)sb.st_atime, (jlong)sb.st_mtime, (jlong)sb.st_ctime,
        (jlong)sb.st_blksize, (jlong)sb.st_blocks);
}

/* access(String path, int mode) */
static jboolean linux_access(JNIEnv* env, jobject thiz, jstring jpath, jint mode) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    int result = access(path, mode);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (result < 0) {
        throwErrnoException(env, "access", errno);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

/* environ() -> String[] */
static jobjectArray linux_environ(JNIEnv* env, jobject thiz) {
    extern char** environ;
    int count = 0;
    while (environ[count]) count++;
    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray(env, count, stringClass, NULL);
    for (int i = 0; i < count; i++) {
        (*env)->SetObjectArrayElement(env, result, i, (*env)->NewStringUTF(env, environ[i]));
    }
    return result;
}

/* setenv(String name, String value, boolean overwrite) */
static void linux_setenv(JNIEnv* env, jobject thiz, jstring jname, jstring jvalue, jboolean overwrite) {
    const char* name = (*env)->GetStringUTFChars(env, jname, NULL);
    const char* value = (*env)->GetStringUTFChars(env, jvalue, NULL);
    setenv(name, value, overwrite ? 1 : 0);
    (*env)->ReleaseStringUTFChars(env, jvalue, value);
    (*env)->ReleaseStringUTFChars(env, jname, name);
}

/* strerror(int errno) */
static jstring linux_strerror(JNIEnv* env, jobject thiz, jint errnum) {
    return (*env)->NewStringUTF(env, strerror(errnum));
}

/* mkdir(String path, int mode) */
static void linux_mkdir(JNIEnv* env, jobject thiz, jstring jpath, jint mode) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (mkdir(path, mode) < 0) {
        throwErrnoException(env, "mkdir", errno);
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

/* rename(String oldPath, String newPath) */
static void linux_rename(JNIEnv* env, jobject thiz, jstring jold, jstring jnew) {
    const char* oldp = (*env)->GetStringUTFChars(env, jold, NULL);
    const char* newp = (*env)->GetStringUTFChars(env, jnew, NULL);
    if (rename(oldp, newp) < 0) {
        throwErrnoException(env, "rename", errno);
    }
    (*env)->ReleaseStringUTFChars(env, jnew, newp);
    (*env)->ReleaseStringUTFChars(env, jold, oldp);
}

/* remove(String path) */
static void linux_remove(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (remove(path) < 0) {
        throwErrnoException(env, "remove", errno);
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

/* unlink(String path) */
static void linux_unlink(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (unlink(path) < 0) {
        throwErrnoException(env, "unlink", errno);
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

/* chmod(String path, int mode) */
static void linux_chmod(JNIEnv* env, jobject thiz, jstring jpath, jint mode) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (chmod(path, mode) < 0) {
        throwErrnoException(env, "chmod", errno);
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

/* fchmod(FileDescriptor fd, int mode) */
static void linux_fchmod(JNIEnv* env, jobject thiz, jobject fdObj, jint mode) {
    int fd = getFd(env, fdObj);
    if (fchmod(fd, mode) < 0) {
        throwErrnoException(env, "fchmod", errno);
    }
}

/* umask(int mask) */
static jint linux_umask(JNIEnv* env, jobject thiz, jint mask) {
    return (jint)umask(mask);
}

/* dup(FileDescriptor fd) -> FileDescriptor */
static jobject linux_dup(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getFd(env, fdObj);
    int newfd = dup(fd);
    if (newfd < 0) {
        throwErrnoException(env, "dup", errno);
        return NULL;
    }
    jclass fdCls = (*env)->FindClass(env, "java/io/FileDescriptor");
    if (!fdCls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, fdCls, "<init>", "()V");
    if (!ctor) return NULL;
    jobject newFdObj = (*env)->NewObject(env, fdCls, ctor);
    if (!newFdObj) return NULL;
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (descField) (*env)->SetIntField(env, newFdObj, descField, newfd);
    return newFdObj;
}

/* dup2(FileDescriptor oldFd, int newFd) -> FileDescriptor */
static jobject linux_dup2(JNIEnv* env, jobject thiz, jobject fdObj, jint newfd) {
    int fd = getFd(env, fdObj);
    int result = dup2(fd, newfd);
    if (result < 0) {
        throwErrnoException(env, "dup2", errno);
        return NULL;
    }
    jclass fdCls = (*env)->FindClass(env, "java/io/FileDescriptor");
    if (!fdCls) return NULL;
    jmethodID ctor = (*env)->GetMethodID(env, fdCls, "<init>", "()V");
    if (!ctor) return NULL;
    jobject newFdObj = (*env)->NewObject(env, fdCls, ctor);
    if (!newFdObj) return NULL;
    jfieldID descField = (*env)->GetFieldID(env, fdCls, "descriptor", "I");
    if (descField) (*env)->SetIntField(env, newFdObj, descField, result);
    return newFdObj;
}

/* ftruncate(FileDescriptor fd, long length) */
static void linux_ftruncate(JNIEnv* env, jobject thiz, jobject fdObj, jlong length) {
    int fd = getFd(env, fdObj);
    if (ftruncate(fd, (off_t)length) < 0) {
        throwErrnoException(env, "ftruncate", errno);
    }
}

/* fsync(FileDescriptor fd) */
static void linux_fsync(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getFd(env, fdObj);
    if (fsync(fd) < 0) {
        throwErrnoException(env, "fsync", errno);
    }
}

/* fdatasync(FileDescriptor fd) */
static void linux_fdatasync(JNIEnv* env, jobject thiz, jobject fdObj) {
    int fd = getFd(env, fdObj);
    if (fdatasync(fd) < 0) {
        throwErrnoException(env, "fdatasync", errno);
    }
}

/* symlink(String oldPath, String newPath) */
static void linux_symlink(JNIEnv* env, jobject thiz, jstring jold, jstring jnew) {
    const char* oldp = (*env)->GetStringUTFChars(env, jold, NULL);
    const char* newp = (*env)->GetStringUTFChars(env, jnew, NULL);
    if (symlink(oldp, newp) < 0) {
        throwErrnoException(env, "symlink", errno);
    }
    (*env)->ReleaseStringUTFChars(env, jnew, newp);
    (*env)->ReleaseStringUTFChars(env, jold, oldp);
}

/* readlink(String path) -> String */
static jstring linux_readlink(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    char buf[PATH_MAX];
    ssize_t n = readlink(path, buf, sizeof(buf) - 1);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (n < 0) {
        throwErrnoException(env, "readlink", errno);
        return NULL;
    }
    buf[n] = '\0';
    return (*env)->NewStringUTF(env, buf);
}

/* realpath(String path) -> String */
static jstring linux_realpath(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = (*env)->GetStringUTFChars(env, jpath, NULL);
    char resolved[PATH_MAX];
    char* result = realpath(path, resolved);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (!result) {
        throwErrnoException(env, "realpath", errno);
        return NULL;
    }
    return (*env)->NewStringUTF(env, resolved);
}

/* getxattr(String path, String name) -> byte[] -- stub: not supported */
static jbyteArray linux_getxattr(JNIEnv* env, jobject thiz, jstring jpath, jstring jname) {
    throwErrnoException(env, "getxattr", ENOTSUP);
    return NULL;
}

/* android_fdsan stubs */
static void linux_android_fdsan_exchange_owner_tag(JNIEnv* env, jobject thiz,
                                                     jobject fd, jlong prev, jlong next) { }
static jlong linux_android_fdsan_get_owner_tag(JNIEnv* env, jobject thiz, jobject fd) { return 0; }
static jstring linux_android_fdsan_get_tag_type(JNIEnv* env, jobject thiz, jlong tag) {
    return (*env)->NewStringUTF(env, "unknown");
}
static jlong linux_android_fdsan_get_tag_value(JNIEnv* env, jobject thiz, jlong tag) { return 0; }

/* fcntlVoid / fcntlInt */
static jint linux_fcntlVoid(JNIEnv* env, jobject thiz, jobject fdObj, jint cmd) {
    int fd = getFd(env, fdObj);
    int result = fcntl(fd, cmd);
    if (result < 0) {
        throwErrnoException(env, "fcntl", errno);
    }
    return result;
}

static jint linux_fcntlInt(JNIEnv* env, jobject thiz, jobject fdObj, jint cmd, jint arg) {
    int fd = getFd(env, fdObj);
    int result = fcntl(fd, cmd, arg);
    if (result < 0) {
        throwErrnoException(env, "fcntl", errno);
    }
    return result;
}

/* ==================== OsConstants native methods ==================== */

static void OsConstants_initConstants(JNIEnv* env, jclass clazz) {
    #define SET_INT(name, val) do { \
        jfieldID fid = (*env)->GetStaticFieldID(env, clazz, #name, "I"); \
        if (fid) { (*env)->SetStaticIntField(env, clazz, fid, val); } \
        else { (*env)->ExceptionClear(env); } \
    } while(0)

    SET_INT(O_RDONLY, O_RDONLY);
    SET_INT(O_WRONLY, O_WRONLY);
    SET_INT(O_RDWR, O_RDWR);
    SET_INT(O_CREAT, O_CREAT);
    SET_INT(O_TRUNC, O_TRUNC);
    SET_INT(O_APPEND, O_APPEND);
    SET_INT(O_EXCL, O_EXCL);
    SET_INT(O_NONBLOCK, O_NONBLOCK);
    SET_INT(O_CLOEXEC, O_CLOEXEC);

    SET_INT(ENOENT, ENOENT);
    SET_INT(EACCES, EACCES);
    SET_INT(EEXIST, EEXIST);
    SET_INT(EBADF, EBADF);
    SET_INT(EINVAL, EINVAL);
    SET_INT(EIO, EIO);
    SET_INT(EISDIR, EISDIR);
    SET_INT(ENOTEMPTY, ENOTEMPTY);
    SET_INT(ENOSPC, ENOSPC);
    SET_INT(EPERM, EPERM);
    SET_INT(EROFS, EROFS);
    SET_INT(ENOMEM, ENOMEM);
    SET_INT(ENOTDIR, ENOTDIR);
    SET_INT(ENAMETOOLONG, ENAMETOOLONG);
    SET_INT(EMFILE, EMFILE);
    SET_INT(ENFILE, ENFILE);
    SET_INT(EINTR, EINTR);
    SET_INT(EAGAIN, EAGAIN);
    SET_INT(EWOULDBLOCK, EWOULDBLOCK);
    SET_INT(ECONNREFUSED, ECONNREFUSED);
    SET_INT(ETIMEDOUT, ETIMEDOUT);
    SET_INT(ECONNRESET, ECONNRESET);
    SET_INT(EPIPE, EPIPE);
    SET_INT(ENOTSOCK, ENOTSOCK);

    SET_INT(S_IFMT, S_IFMT);
    SET_INT(S_IFREG, S_IFREG);
    SET_INT(S_IFDIR, S_IFDIR);
    SET_INT(S_IFLNK, S_IFLNK);
    SET_INT(S_IFCHR, S_IFCHR);
    SET_INT(S_IFBLK, S_IFBLK);
    SET_INT(S_IFIFO, S_IFIFO);
    SET_INT(S_IFSOCK, S_IFSOCK);
    SET_INT(S_ISUID, S_ISUID);
    SET_INT(S_ISGID, S_ISGID);
    SET_INT(S_IRUSR, S_IRUSR);
    SET_INT(S_IWUSR, S_IWUSR);
    SET_INT(S_IXUSR, S_IXUSR);
    SET_INT(S_IRGRP, S_IRGRP);
    SET_INT(S_IWGRP, S_IWGRP);
    SET_INT(S_IXGRP, S_IXGRP);
    SET_INT(S_IROTH, S_IROTH);
    SET_INT(S_IWOTH, S_IWOTH);
    SET_INT(S_IXOTH, S_IXOTH);

    SET_INT(F_OK, F_OK);
    SET_INT(R_OK, R_OK);
    SET_INT(W_OK, W_OK);
    SET_INT(X_OK, X_OK);

    SET_INT(SEEK_SET, SEEK_SET);
    SET_INT(SEEK_CUR, SEEK_CUR);
    SET_INT(SEEK_END, SEEK_END);

    SET_INT(F_GETFD, F_GETFD);
    SET_INT(F_SETFD, F_SETFD);
    SET_INT(F_GETFL, F_GETFL);
    SET_INT(F_SETFL, F_SETFL);
    SET_INT(FD_CLOEXEC, FD_CLOEXEC);

    SET_INT(STDIN_FILENO, STDIN_FILENO);
    SET_INT(STDOUT_FILENO, STDOUT_FILENO);
    SET_INT(STDERR_FILENO, STDERR_FILENO);

    #undef SET_INT
}

/* ==================== JNI_OnLoad ==================== */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) return -1;

    /* Register native methods for libcore.io.Linux */
    jclass linuxClass = (*env)->FindClass(env, "libcore/io/Linux");
    if (linuxClass) {
        JNINativeMethod methods[] = {
            {"getpwuid", "(I)Landroid/system/StructPasswd;", (void*)linux_getpwuid},
            {"uname", "()Landroid/system/StructUtsname;", (void*)linux_uname},
            {"getenv", "(Ljava/lang/String;)Ljava/lang/String;", (void*)linux_getenv},
            {"getuid", "()I", (void*)linux_getuid},
            {"getpid", "()I", (void*)linux_getpid},
            {"geteuid", "()I", (void*)linux_geteuid},
            {"getegid", "()I", (void*)linux_getegid},
            {"getgid", "()I", (void*)linux_getgid},
            {"getppid", "()I", (void*)linux_getppid},
            {"sysconf", "(I)J", (void*)linux_sysconf},
            {"isatty", "(Ljava/io/FileDescriptor;)Z", (void*)linux_isatty},
            {"writeBytes", "(Ljava/io/FileDescriptor;Ljava/lang/Object;II)I", (void*)linux_writeBytes},
            {"readBytes", "(Ljava/io/FileDescriptor;Ljava/lang/Object;II)I", (void*)linux_readBytes},
            {"close", "(Ljava/io/FileDescriptor;)V", (void*)linux_close},
            {"open", "(Ljava/lang/String;II)Ljava/io/FileDescriptor;", (void*)linux_open},
            {"lseek", "(Ljava/io/FileDescriptor;JI)J", (void*)linux_lseek},
            {"fstat", "(Ljava/io/FileDescriptor;)Landroid/system/StructStat;", (void*)linux_fstat},
            {"stat", "(Ljava/lang/String;)Landroid/system/StructStat;", (void*)linux_stat},
            {"lstat", "(Ljava/lang/String;)Landroid/system/StructStat;", (void*)linux_lstat},
            {"access", "(Ljava/lang/String;I)Z", (void*)linux_access},
            {"environ", "()[Ljava/lang/String;", (void*)linux_environ},
            {"setenv", "(Ljava/lang/String;Ljava/lang/String;Z)V", (void*)linux_setenv},
            {"strerror", "(I)Ljava/lang/String;", (void*)linux_strerror},
            {"android_fdsan_exchange_owner_tag", "(Ljava/io/FileDescriptor;JJ)V", (void*)linux_android_fdsan_exchange_owner_tag},
            {"android_fdsan_get_owner_tag", "(Ljava/io/FileDescriptor;)J", (void*)linux_android_fdsan_get_owner_tag},
            {"android_fdsan_get_tag_type", "(J)Ljava/lang/String;", (void*)linux_android_fdsan_get_tag_type},
            {"android_fdsan_get_tag_value", "(J)J", (void*)linux_android_fdsan_get_tag_value},
            {"fcntlVoid", "(Ljava/io/FileDescriptor;I)I", (void*)linux_fcntlVoid},
            {"fcntlInt", "(Ljava/io/FileDescriptor;II)I", (void*)linux_fcntlInt},
            {"mkdir", "(Ljava/lang/String;I)V", (void*)linux_mkdir},
            {"rename", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)linux_rename},
            {"remove", "(Ljava/lang/String;)V", (void*)linux_remove},
            {"unlink", "(Ljava/lang/String;)V", (void*)linux_unlink},
            {"chmod", "(Ljava/lang/String;I)V", (void*)linux_chmod},
            {"fchmod", "(Ljava/io/FileDescriptor;I)V", (void*)linux_fchmod},
            {"umaskImpl", "(I)I", (void*)linux_umask},
            {"dup", "(Ljava/io/FileDescriptor;)Ljava/io/FileDescriptor;", (void*)linux_dup},
            {"dup2", "(Ljava/io/FileDescriptor;I)Ljava/io/FileDescriptor;", (void*)linux_dup2},
            {"ftruncate", "(Ljava/io/FileDescriptor;J)V", (void*)linux_ftruncate},
            {"fsync", "(Ljava/io/FileDescriptor;)V", (void*)linux_fsync},
            {"fdatasync", "(Ljava/io/FileDescriptor;)V", (void*)linux_fdatasync},
            {"symlink", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)linux_symlink},
            {"readlink", "(Ljava/lang/String;)Ljava/lang/String;", (void*)linux_readlink},
            {"realpath", "(Ljava/lang/String;)Ljava/lang/String;", (void*)linux_realpath},
            {"getxattr", "(Ljava/lang/String;Ljava/lang/String;)[B", (void*)linux_getxattr},
        };
        registerNativesOrSkip(env, linuxClass, methods, sizeof(methods)/sizeof(methods[0]));
        (*env)->DeleteLocalRef(env, linuxClass);
    }

    /* Register OsConstants.initConstants() */
    {
        jclass cls = (*env)->FindClass(env, "android/system/OsConstants");
        if (cls) {
            JNINativeMethod methods[] = {
                {"initConstants", "()V", (void*)OsConstants_initConstants},
            };
            registerNativesOrSkip(env, cls, methods, 1);
            (*env)->DeleteLocalRef(env, cls);
        }
    }

    return JNI_VERSION_1_6;
}
