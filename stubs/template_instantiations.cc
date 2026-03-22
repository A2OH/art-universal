// Explicit template instantiations for art::mirror types.
// Provides definitions for template specializations needed by the linker.

#pragma clang diagnostic ignored "-Weverything"

#include "mirror/object_array-inl.h"
#include "mirror/object-inl.h"
#include "mirror/class-inl.h"
#include "mirror/dex_cache-inl.h"
#include "mirror/string-inl.h"
#include "mirror/method.h"
#include "mirror/stack_trace_element.h"
#include "mirror/method_handle_impl.h"
#include "mirror/method_type.h"
#include "handle.h"

namespace art {
namespace mirror {

// GetWithoutChecks
template ObjPtr<ClassLoader> ObjectArray<ClassLoader>::GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(int32_t);
template ObjPtr<DexCache> ObjectArray<DexCache>::GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(int32_t);
template ObjPtr<Method> ObjectArray<Method>::GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(int32_t);
template ObjPtr<Object> ObjectArray<Object>::GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(int32_t);

// Get
template ObjPtr<ObjectArray<String>> ObjectArray<ObjectArray<String>>::Get<kDefaultVerifyFlags, kWithoutReadBarrier>(int32_t);
template ObjPtr<StackTraceElement> ObjectArray<StackTraceElement>::Get<kDefaultVerifyFlags, kWithoutReadBarrier>(int32_t);

// Set (2-arg)
template void ObjectArray<Class>::Set(int32_t, ObjPtr<Class>);
template void ObjectArray<ClassLoader>::Set(int32_t, ObjPtr<ClassLoader>);
template void ObjectArray<Object>::Set(int32_t, ObjPtr<Object>);

// Set (template args)
template void ObjectArray<Class>::Set<false, true, kDefaultVerifyFlags>(int32_t, ObjPtr<Class>);
template void ObjectArray<Class>::Set<true, true, kDefaultVerifyFlags>(int32_t, ObjPtr<Class>);
template void ObjectArray<ObjectArray<Class>>::Set<false, false, kDefaultVerifyFlags>(int32_t, ObjPtr<ObjectArray<Class>>);
template void ObjectArray<Array>::Set<false, false, kDefaultVerifyFlags>(int32_t, ObjPtr<Array>);
template void ObjectArray<MethodHandle>::Set<false, true, kDefaultVerifyFlags>(int32_t, ObjPtr<MethodHandle>);
template void ObjectArray<MethodHandle>::Set<true, true, kDefaultVerifyFlags>(int32_t, ObjPtr<MethodHandle>);
template void ObjectArray<MethodType>::Set<false, true, kDefaultVerifyFlags>(int32_t, ObjPtr<MethodType>);
template void ObjectArray<MethodType>::Set<true, true, kDefaultVerifyFlags>(int32_t, ObjPtr<MethodType>);
template void ObjectArray<StackTraceElement>::Set<false, true, kDefaultVerifyFlags>(int32_t, ObjPtr<StackTraceElement>);
template void ObjectArray<String>::Set<false, true, kDefaultVerifyFlags>(int32_t, ObjPtr<String>);
template void ObjectArray<String>::Set<true, true, kDefaultVerifyFlags>(int32_t, ObjPtr<String>);

// Misc
template MemberOffset ObjectArray<Object>::OffsetOfElement(int32_t);
template void ObjectArray<Object>::AssignableCheckingMemcpy<true>(int32_t, ObjPtr<ObjectArray<Object>>, int32_t, int32_t, bool);
template ObjPtr<String> Object::AsString<kDefaultVerifyFlags>();
template bool Object::IsString<kDefaultVerifyFlags>();

// VisitReferences - instantiate for all GC visitor types that are referenced
// (These are defined as template methods on ObjectArray in object_array-inl.h)
// We use extern template to suppress instantiation everywhere else, then
// provide it here via a compilation unit that includes all needed headers.

// DexCache atomics are provided via separate stub file

}  // namespace mirror
}  // namespace art

// For the non-template methods (begin/end/cbegin/cend/CheckIdx/DexCache atomics),
// we can't use explicit instantiation. Instead, we create a helper TU that
// forces the compiler to emit them by including the inl.h and using them.
// This is done by defining dummy functions that reference them.

namespace {
using namespace art;
using namespace art::mirror;

// This function is never called but forces the compiler to emit the needed symbols.
__attribute__((used, optnone))
void force_mirror_instantiations() {
  // ObjectArray iterators - these are non-template member functions that
  // only exist when ObjectArray<T> is instantiated for specific T.
  // We force their emission by calling them on fake objects.

  // Use reinterpret_cast to avoid constructor issues
  auto& cl_arr = *reinterpret_cast<ObjectArray<ClassLoader>*>(8);
  auto& dc_arr = *reinterpret_cast<ObjectArray<DexCache>*>(8);
  auto& me_arr = *reinterpret_cast<ObjectArray<Method>*>(8);
  auto& ob_arr = *reinterpret_cast<ObjectArray<Object>*>(8);

  auto& h_cl = *reinterpret_cast<Handle<ObjectArray<ClassLoader>>*>(8);
  auto& h_dc = *reinterpret_cast<Handle<ObjectArray<DexCache>>*>(8);
  auto& h_me = *reinterpret_cast<Handle<ObjectArray<Method>>*>(8);
  auto& h_ob = *reinterpret_cast<Handle<ObjectArray<Object>>*>(8);

  // begin/end with Handle parameter - dereference iterators to force CheckIdx emission
  { auto it = cl_arr.begin(h_cl); (void)*it; cl_arr.end(h_cl); }
  { auto it = dc_arr.begin(); (void)*it; dc_arr.end(); }
  { auto it = dc_arr.begin(h_dc); (void)*it; dc_arr.end(h_dc); }
  { auto it = me_arr.begin(h_me); (void)*it; me_arr.end(h_me); }
  { auto it = ob_arr.begin(h_ob); (void)*it; ob_arr.end(h_ob); }

  // cbegin/cend with const Handle
  { const auto& ch_dc = h_dc; auto it = dc_arr.cbegin(ch_dc); (void)*it; dc_arr.cend(ch_dc); }

  // DexCache begin iter with ObjPtr - not an actual overload, skip
}
}  // anonymous namespace
