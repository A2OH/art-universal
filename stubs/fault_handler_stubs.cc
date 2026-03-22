// Stubs for arch-specific fault handler virtual methods.
// The constructors are in the real fault_handler.cc; only the
// arch-specific Action() methods and GetMethodAndReturnPcAndSp()
// need stubs since fault_handler_x86.cc can't compile.
// dex2oat never triggers these handlers.

#include "fault_handler.h"
#include <signal.h>

namespace art {

bool NullPointerHandler::Action(int, siginfo_t*, void*) {
  return false;
}

bool SuspensionHandler::Action(int, siginfo_t*, void*) {
  return false;
}

bool StackOverflowHandler::Action(int, siginfo_t*, void*) {
  return false;
}

void FaultManager::GetMethodAndReturnPcAndSp(siginfo_t*, void*,
                                              ArtMethod**, uintptr_t*, uintptr_t*, bool*) {
}

}  // namespace art
