// Shadow gc/space/image_space.h - includes the real header plus gc/heap.h
// to fix incomplete type error for gc::Heap in code_generator.cc
#ifndef STUB_GC_SPACE_IMAGE_SPACE_WRAPPER_H_
#define STUB_GC_SPACE_IMAGE_SPACE_WRAPPER_H_

#pragma GCC system_header
#include_next "gc/space/image_space.h"
#include "gc/heap.h"

#endif  // STUB_GC_SPACE_IMAGE_SPACE_WRAPPER_H_
