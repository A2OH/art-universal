// sigchain_musl.cc - Minimal signal chain for musl/OHOS static builds.
// The real sigchain.cc dlopen's libc to intercept signal handlers,
// which doesn't work in static builds. This provides pass-through stubs.

#include <signal.h>
#include <cstdlib>
#include <cstdio>

extern "C" {

// These are the sigchain API functions that ART calls.

void EnsureFrontOfChain(int signal) {
    // In a static build, we are the only signal handler. Nothing to do.
    (void)signal;
}

void AddSpecialSignalHandlerFn(int signal, void* sa) {
    // Register the handler directly via sigaction.
    if (sa) {
        struct sigaction* act = reinterpret_cast<struct sigaction*>(sa);
        sigaction(signal, act, nullptr);
    }
}

void RemoveSpecialSignalHandlerFn(int signal, bool (*fn)(int, siginfo_t*, void*)) {
    (void)signal;
    (void)fn;
}

// InitializeSignalChain - called during Runtime::Init()
void InitializeSignalChain() {
    // Nothing to initialize in static builds.
}

// SkipAddSignalHandler - used by some code paths
void SkipAddSignalHandler(bool skip) {
    (void)skip;
}

}  // extern "C"
