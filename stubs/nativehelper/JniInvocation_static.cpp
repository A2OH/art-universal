// JniInvocation_static.cpp - Static-link version of JniInvocation.
// Instead of dlopen("libart.so"), directly calls the statically-linked
// JNI_CreateJavaVM / JNI_GetDefaultJavaVMInitArgs / JNI_GetCreatedJavaVMs
// from the ART runtime (art/runtime/jni/java_vm_ext.cc).

#include "nativehelper/JniInvocation.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "jni.h"
#include "JniConstants.h"

// These are defined in art/runtime/jni/java_vm_ext.cc, linked statically.
extern "C" jint JNI_CreateJavaVM(JavaVM** p_vm, JNIEnv** p_env, void* vm_args);
extern "C" jint JNI_GetDefaultJavaVMInitArgs(void* vm_args);
extern "C" jint JNI_GetCreatedJavaVMs(JavaVM** vms, jsize size, jsize* vm_count);

// Internal implementation - bypass dlopen entirely
struct JniInvocationImpl final {
public:
    JniInvocationImpl();
    ~JniInvocationImpl();
    bool Init(const char* library);
    static const char* GetLibrary(const char* library, char* buffer,
                                  bool (*is_debuggable)() = nullptr,
                                  int (*get_library_system_property)(char*) = nullptr);
    static JniInvocationImpl& GetJniInvocation();
    static bool IsInitialized();

    jint DoGetDefaultJavaVMInitArgs(void* vmargs);
    jint DoCreateJavaVM(JavaVM** p_vm, JNIEnv** p_env, void* vm_args);
    jint DoGetCreatedJavaVMs(JavaVM** vms, jsize size, jsize* vm_count);

private:
    static JniInvocationImpl* jni_invocation_;
};

// No vtable, minimal struct for static linking.

JniInvocationImpl* JniInvocationImpl::jni_invocation_ = nullptr;

JniInvocationImpl::JniInvocationImpl() {
    if (jni_invocation_ != nullptr) {
        fprintf(stderr, "JniInvocation: already initialized\n");
        abort();
    }
    jni_invocation_ = this;
}

JniInvocationImpl::~JniInvocationImpl() {
    jni_invocation_ = nullptr;
}

bool JniInvocationImpl::Init(const char* library) {
    // No dlopen needed - everything is statically linked
    (void)library;
    return true;
}

const char* JniInvocationImpl::GetLibrary(const char* library, char* buffer,
                                          bool (*is_debuggable)(),
                                          int (*get_library_system_property)(char*)) {
    (void)buffer;
    (void)is_debuggable;
    (void)get_library_system_property;
    return library ? library : "libart.so";
}

JniInvocationImpl& JniInvocationImpl::GetJniInvocation() {
    if (jni_invocation_ == nullptr) {
        fprintf(stderr, "JniInvocation not initialized\n");
        abort();
    }
    return *jni_invocation_;
}

bool JniInvocationImpl::IsInitialized() {
    return jni_invocation_ != nullptr;
}

jint JniInvocationImpl::DoGetDefaultJavaVMInitArgs(void* vmargs) {
    return ::JNI_GetDefaultJavaVMInitArgs(vmargs);
}

jint JniInvocationImpl::DoCreateJavaVM(JavaVM** p_vm, JNIEnv** p_env, void* vm_args) {
    return ::JNI_CreateJavaVM(p_vm, p_env, vm_args);
}

jint JniInvocationImpl::DoGetCreatedJavaVMs(JavaVM** vms, jsize size, jsize* vm_count) {
    return ::JNI_GetCreatedJavaVMs(vms, size, vm_count);
}

// === Global JNI functions that dalvikvm.cc calls ===
// Note: These OVERRIDE the ones from java_vm_ext.cc because the linker
// picks the first definition. The ones in java_vm_ext.cc are weak/overridable
// due to being extern "C". We need these wrappers to call JniConstants::Uninitialize().

// Actually, we do NOT redefine JNI_CreateJavaVM etc here because the runtime
// already provides them and dalvikvm.cc calls them directly.
// JniInvocation::Init() is what dalvikvm calls first, then JNI_CreateJavaVM directly.

// === JniInvocation C++ wrapper class (from nativehelper/JniInvocation.h) ===
// The header defines JniInvocation as a class with Init(), GetLibrary() etc.

const char* JniInvocation::GetLibrary(const char* library, char* buffer,
                                      bool (*is_debuggable)(),
                                      int (*get_library_system_property)(char*)) {
    return JniInvocationImpl::GetLibrary(library, buffer, is_debuggable, get_library_system_property);
}

// === C API ===
JniInvocationImpl* JniInvocationCreate() {
    return new JniInvocationImpl();
}

void JniInvocationDestroy(JniInvocationImpl* instance) {
    delete instance;
}

int JniInvocationInit(JniInvocationImpl* instance, const char* library) {
    return instance->Init(library) ? 1 : 0;
}

const char* JniInvocationGetLibrary(const char* library, char* buffer) {
    return JniInvocationImpl::GetLibrary(library, buffer);
}
