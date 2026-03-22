import re

with open('compiler_driver_patched.cc', 'r') as f:
    content = f.read()

# Replace the compile check to allow compilation when verified_method is null
old = """      bool compile =
          // Basic checks, e.g., not <clinit>.
          results->IsCandidateForCompilation(method_ref, access_flags) &&
          // Did not fail to create VerifiedMethod metadata.
          verified_method != nullptr &&
          // Do not have failures that should punt to the interpreter.
          !verified_method->HasRuntimeThrow() &&
          (verified_method->GetEncounteredVerificationFailures() &
              (verifier::VERIFY_ERROR_FORCE_INTERPRETER | verifier::VERIFY_ERROR_LOCKING)) == 0 &&
              // Is eligable for compilation by methods-to-compile filter.
              driver->ShouldCompileBasedOnProfile(method_ref);"""

new = """      bool compile =
          // Basic checks, e.g., not <clinit>.
          results->IsCandidateForCompilation(method_ref, access_flags) &&
          // Allow compilation even without verified method (for -Xverify:none builds)
          (verified_method == nullptr ||
           (!verified_method->HasRuntimeThrow() &&
            (verified_method->GetEncounteredVerificationFailures() &
                (verifier::VERIFY_ERROR_FORCE_INTERPRETER | verifier::VERIFY_ERROR_LOCKING)) == 0)) &&
              // Is eligable for compilation by methods-to-compile filter.
              driver->ShouldCompileBasedOnProfile(method_ref);"""

content = content.replace(old, new)

with open('compiler_driver_patched.cc', 'w') as f:
    f.write(content)
print("Patched successfully")
