ART = /home/dspfac/aosp-android-11/art
AOSP = /home/dspfac/aosp-android-11
STUBS = /home/dspfac/art-universal-build/stubs

# Use AOSP clang 11 + libc++ (matches AOSP build environment)
# This avoids GCC 13 issues: cmpxchg16b asm, constexpr, iterator types
CXX = $(AOSP)/prebuilts/clang/host/linux-x86/clang-r383902b1/bin/clang++
LIBCXX = $(AOSP)/prebuilts/clang/host/linux-x86/clang-r383902b1/include/c++/v1

CXXFLAGS = -std=c++17 -O2 -w -mcx16 -fPIC -DNDEBUG \
  -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) \
  -include $(STUBS)/art_gcc_compat.h \
  -Wno-attributes \
  -I$(STUBS) \
  -I$(STUBS)/runtime \
  -I$(ART) \
  -I$(ART)/libdexfile \
  -I$(ART)/libartbase \
  -I$(ART)/libartpalette/include \
  -isystem $(ART)/runtime \
  -I$(ART)/compiler \
  -I$(ART)/disassembler \
  -I$(ART)/compiler/debug \
  -I$(ART)/libelffile \
  -I$(ART)/libprofile \
  -I$(ART)/cmdline \
  -I$(ART)/dex2oat \
  -I$(ART)/dex2oat/include \
  -I$(ART)/dexlayout \
  -I$(AOSP)/system/core/base/include \
  -I$(AOSP)/system/core/libziparchive/include \
  -I$(AOSP)/system/core/libutils/include \
  -I$(AOSP)/system/core/libcutils/include \
  -I$(AOSP)/libnativehelper/include_jni \
  -I$(AOSP)/libnativehelper/header_only_include \
  -I$(AOSP)/libnativehelper/include \
  -I$(AOSP)/system/logging/liblog/include \
  -I$(AOSP)/prebuilts/jdk/jdk11/linux-x86/include \
  -I$(AOSP)/prebuilts/jdk/jdk11/linux-x86/include/linux \
  -I$(AOSP)/external/zlib \
  -I$(AOSP)/external/lz4/lib \
  -I$(AOSP)/external/vixl/src \
  -I$(AOSP)/external/lzma/C \
  -I$(ART)/sigchainlib \
  -I$(AOSP)/libnativehelper/platform_include \
  -I$(ART)/libnativebridge/include \
  -I$(AOSP)/system/core/libbacktrace/include \
  -I$(ART)/libnativeloader/include \
  -I$(AOSP)/external/icu/icu4c/source/common \
  -DART_DEFAULT_GC_TYPE_IS_CMS \
  -DBUILDING_LIBART \
  -DART_BASE_ADDRESS=0x70000000 \
  -DART_STACK_OVERFLOW_GAP_arm=8192 \
  -DART_STACK_OVERFLOW_GAP_arm64=8192 \
  -DART_STACK_OVERFLOW_GAP_x86=8192 \
  -DART_STACK_OVERFLOW_GAP_x86_64=8192 \
  -DUSE_D8_DESUGAR \
  -DART_USE_CXX_INTERPRETER

BUILDDIR = build

# For linking: use same clang + libc++ to match compilation ABI
HOSTLD = $(CXX) -stdlib=libc++
CLANG_LIB = $(AOSP)/prebuilts/clang/host/linux-x86/clang-r383902b1/lib64
SYSROOT = $(AOSP)/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8/sysroot
LDFLAGS = -L$(CLANG_LIB) -Wl,-rpath,$(CLANG_LIB) \
  -lc++ \
  $(SYSROOT)/usr/lib/libz.a -lpthread -ldl -lm

# ============ libdexfile ============
DEXFILE_SRCS = $(filter-out %_test.cc %_fuzzer.cc,$(wildcard $(ART)/libdexfile/dex/*.cc))
DEXFILE_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(DEXFILE_SRCS))

# ============ libartbase ============
# Exclude platform-specific files (fuchsia, windows) and safe_copy (needs sys/ucontext)
ARTBASE_SRCS_ALL = $(filter-out %_test.cc %_fuzzer.cc,$(wildcard \
  $(ART)/libartbase/base/*.cc \
  $(ART)/libartbase/base/unix_file/*.cc \
  $(ART)/libartbase/arch/*.cc))
# Exclude platform-specific
ARTBASE_EXCLUDE = %mem_map_fuchsia.cc %mem_map_windows.cc %safe_copy.cc %socket_peer_is_trusted.cc
ARTBASE_SRCS = $(filter-out $(ARTBASE_EXCLUDE),$(ARTBASE_SRCS_ALL))
ARTBASE_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(ARTBASE_SRCS))

# ============ compiler ============
COMPILER_SRCS_ALL = $(filter-out %_test.cc %_fuzzer.cc,$(wildcard \
  $(ART)/compiler/*.cc \
  $(ART)/compiler/debug/*.cc \
  $(ART)/compiler/debug/dwarf/*.cc \
  $(ART)/compiler/dex/*.cc \
  $(ART)/compiler/driver/*.cc \
  $(ART)/compiler/jit/*.cc \
  $(ART)/compiler/jni/*.cc \
  $(ART)/compiler/jni/quick/*.cc \
  $(ART)/compiler/jni/quick/arm/*.cc \
  $(ART)/compiler/jni/quick/arm64/*.cc \
  $(ART)/compiler/jni/quick/x86/*.cc \
  $(ART)/compiler/jni/quick/x86_64/*.cc \
  $(ART)/compiler/linker/*.cc \
  $(ART)/compiler/linker/x86/*.cc \
  $(ART)/compiler/linker/x86_64/*.cc \
  $(ART)/compiler/linker/arm/*.cc \
  $(ART)/compiler/linker/arm64/*.cc \
  $(ART)/compiler/optimizing/*.cc \
  $(ART)/compiler/trampolines/*.cc \
  $(ART)/compiler/utils/*.cc \
  $(ART)/compiler/utils/x86/*.cc \
  $(ART)/compiler/utils/x86_64/*.cc \
  $(ART)/compiler/utils/arm/*.cc \
  $(ART)/compiler/utils/arm64/*.cc))
COMPILER_SRCS = $(COMPILER_SRCS_ALL)
COMPILER_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(COMPILER_SRCS))

# ============ dex2oat ============
DEX2OAT_SRCS = $(filter-out %_test.cc %_fuzzer.cc,$(wildcard \
  $(ART)/dex2oat/*.cc \
  $(ART)/dex2oat/dex/*.cc \
  $(ART)/dex2oat/driver/*.cc \
  $(ART)/dex2oat/linker/*.cc \
  $(ART)/dex2oat/linker/arm/*.cc \
  $(ART)/dex2oat/linker/arm64/*.cc \
  $(ART)/dex2oat/linker/x86/*.cc \
  $(ART)/dex2oat/linker/x86_64/*.cc))
DEX2OAT_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(DEX2OAT_SRCS))

# ============ runtime ============
RUNTIME_SRCS_ALL = $(filter-out %_test.cc %_fuzzer.cc %_bench.cc,$(wildcard \
  $(ART)/runtime/*.cc \
  $(ART)/runtime/arch/*.cc \
  $(ART)/runtime/arch/arm/*.cc \
  $(ART)/runtime/arch/arm64/*.cc \
  $(ART)/runtime/arch/x86/*.cc \
  $(ART)/runtime/arch/x86_64/*.cc \
  $(ART)/runtime/base/*.cc \
  $(ART)/runtime/entrypoints/*.cc \
  $(ART)/runtime/entrypoints/quick/*.cc \
  $(ART)/runtime/gc/*.cc \
  $(ART)/runtime/gc/accounting/*.cc \
  $(ART)/runtime/gc/allocator/*.cc \
  $(ART)/runtime/gc/collector/*.cc \
  $(ART)/runtime/gc/space/*.cc \
  $(ART)/runtime/interpreter/*.cc \
  $(ART)/runtime/interpreter/mterp/*.cc \
  $(ART)/runtime/jit/*.cc \
  $(ART)/runtime/jni/*.cc \
  $(ART)/runtime/mirror/*.cc \
  $(ART)/runtime/native/*.cc \
  $(ART)/runtime/dex/*.cc \
  $(ART)/runtime/ti/*.cc \
  $(ART)/runtime/verifier/*.cc))
RUNTIME_EXCLUDE = %backtrace_helper.cc \
  %native_stack_dump.cc \
  %thread_linux.cc %asm_support_check.cc \
  %runtime_android.cc \
  %arch/arm/thread_arm.cc %arch/arm64/thread_arm64.cc %arch/x86/thread_x86.cc \
  %mterp/nterp_stub.cc %mterp/mterp_stub.cc \
  %arch/arm/entrypoints_init_arm.cc %arch/arm64/entrypoints_init_arm64.cc \
  %arch/x86/entrypoints_init_x86.cc \
  %arch/arm/context_arm.cc %arch/arm64/context_arm64.cc %arch/x86/context_x86.cc \
  %arch/arm/fault_handler_arm.cc %arch/arm64/fault_handler_arm64.cc %arch/x86/fault_handler_x86.cc \
  %arch/arm/quick_entrypoints_cc_arm.cc \
  %monitor_android.cc
RUNTIME_SRCS = $(filter-out $(RUNTIME_EXCLUDE),$(RUNTIME_SRCS_ALL))
RUNTIME_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(RUNTIME_SRCS))

# ============ dexlayout ============
DEXLAYOUT_SRCS = $(filter-out %_test.cc %_fuzzer.cc %dexlayout_main.cc %dexdiag.cc %dexdump_main.cc,$(wildcard $(ART)/dexlayout/*.cc))
DEXLAYOUT_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(DEXLAYOUT_SRCS))

# ============ libelffile ============
ELFFILE_SRCS = $(filter-out %_test.cc %xz_utils.cc,$(wildcard $(ART)/libelffile/elf/*.cc $(ART)/libelffile/stream/*.cc))
ELFFILE_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(ELFFILE_SRCS))

# ============ libprofile ============
PROFILE_SRCS = $(filter-out %_test.cc,$(wildcard $(ART)/libprofile/profile/*.cc))
PROFILE_OBJS = $(patsubst $(ART)/%.cc,$(BUILDDIR)/%.o,$(PROFILE_SRCS))

# ============ VIXL (ARM assembler) ============
VIXL_SRCS = $(filter-out %_test.cc %test-runner.cc,$(wildcard \
  $(AOSP)/external/vixl/src/*.cc \
  $(AOSP)/external/vixl/src/aarch32/*.cc \
  $(AOSP)/external/vixl/src/aarch64/*.cc))
VIXL_OBJS = $(patsubst $(AOSP)/external/vixl/%.cc,$(BUILDDIR)/vixl/%.o,$(VIXL_SRCS))
VIXL_CXXFLAGS = -std=c++17 -O2 -w -fPIC \
  -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) \
  -I$(AOSP)/external/vixl/src \
  -DVIXL_INCLUDE_TARGET_AARCH32 \
  -DVIXL_INCLUDE_TARGET_AARCH64 \
  -DVIXL_CODE_BUFFER_MMAP

# ============ android-base ============
ABASE_SRCS = $(filter-out %_test.cpp %_benchmark.cpp %test_main.cpp %test_utils.cpp %test_utils_test.cpp,$(wildcard \
  $(AOSP)/system/core/base/*.cpp))
ABASE_EXCLUDE = %chrono_utils.cpp %chrono_utils_test.cpp %process.cpp %process_test.cpp \
  %properties.cpp %properties_test.cpp %errors_windows.cpp %format_benchmark.cpp \
  %utf8.cpp
ABASE_SRCS_FILTERED = $(filter-out $(ABASE_EXCLUDE),$(ABASE_SRCS))
ABASE_OBJS = $(patsubst $(AOSP)/system/core/base/%.cpp,$(BUILDDIR)/android-base/%.o,$(ABASE_SRCS_FILTERED))
ABASE_CXXFLAGS = -std=c++17 -O2 -w -fPIC \
  -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) \
  -I$(AOSP)/system/core/base/include \
  -I$(AOSP)/system/logging/liblog/include \
  -I$(AOSP)/system/core/liblog/include \
  -I$(AOSP)/system/core/libcutils/include

# ============ link stubs (C libraries) ============
STUBS_OBJ = $(BUILDDIR)/stubs/link_stubs.o

# ============ targets ============
.PHONY: all libdexfile libartbase libcompiler dex2oat-build runtime dexlayout libelffile libprofile vixl android-base link-stubs link link-runtime nativehelper dalvikvm-main clean status

all: libdexfile libartbase libcompiler dex2oat-build runtime dexlayout libelffile libprofile vixl android-base link-stubs

libdexfile: $(DEXFILE_OBJS)
	@echo "=== libdexfile ==="
	@echo "Compiled: $$(ls $(BUILDDIR)/libdexfile/dex/*.o 2>/dev/null | wc -l) / $(words $(DEXFILE_OBJS))"

libartbase: $(ARTBASE_OBJS)
	@echo "=== libartbase ==="
	@echo "Compiled: $$(ls $(BUILDDIR)/libartbase/base/*.o $(BUILDDIR)/libartbase/base/unix_file/*.o $(BUILDDIR)/libartbase/arch/*.o 2>/dev/null | wc -l) / $(words $(ARTBASE_OBJS))"

libcompiler: $(COMPILER_OBJS)
	@echo "=== compiler ==="
	@compiled=$$(find $(BUILDDIR)/compiler -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(COMPILER_OBJS))"

dex2oat-build: $(DEX2OAT_OBJS)
	@echo "=== dex2oat ==="
	@compiled=$$(find $(BUILDDIR)/dex2oat -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(DEX2OAT_OBJS))"

vixl: $(VIXL_OBJS)
	@echo "=== VIXL ==="
	@compiled=$$(find $(BUILDDIR)/vixl -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(VIXL_OBJS))"

android-base: $(ABASE_OBJS)
	@echo "=== android-base ==="
	@compiled=$$(ls $(BUILDDIR)/android-base/*.o 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(ABASE_OBJS))"

runtime: $(RUNTIME_OBJS)
	@echo "=== runtime ==="
	@compiled=$$(find $(BUILDDIR)/runtime -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(RUNTIME_OBJS))"

dexlayout: $(DEXLAYOUT_OBJS)
	@echo "=== dexlayout ==="
	@compiled=$$(find $(BUILDDIR)/dexlayout -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(DEXLAYOUT_OBJS))"

libelffile: $(ELFFILE_OBJS)
	@echo "=== libelffile ==="
	@compiled=$$(find $(BUILDDIR)/libelffile -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(ELFFILE_OBJS))"

libprofile: $(PROFILE_OBJS)
	@echo "=== libprofile ==="
	@compiled=$$(find $(BUILDDIR)/libprofile -name '*.o' 2>/dev/null | wc -l); \
	echo "Compiled: $$compiled / $(words $(PROFILE_OBJS))"

link-stubs: $(STUBS_OBJ)
	@echo "=== link stubs ==="
	@echo "Compiled: link_stubs.o"

link: all ziparchive sigchain
	@echo "=== Linking dex2oat ==="
	@mkdir -p $(BUILDDIR)/bin
	$(HOSTLD) -o $(BUILDDIR)/bin/dex2oat \
	  -Wl,--unresolved-symbols=ignore-in-object-files \
	  $$(find $(BUILDDIR)/dex2oat -name '*.o') \
	  $$(find $(BUILDDIR)/compiler -name '*.o') \
	  $$(find $(BUILDDIR)/runtime -name '*.o') \
	  $$(find $(BUILDDIR)/libdexfile -name '*.o') \
	  $$(find $(BUILDDIR)/libartbase -name '*.o') \
	  $$(find $(BUILDDIR)/libelffile -name '*.o') \
	  $$(find $(BUILDDIR)/libprofile -name '*.o') \
	  $$(find $(BUILDDIR)/dexlayout -name '*.o') \
	  $$(find $(BUILDDIR)/vixl -name '*.o') \
	  $$(find $(BUILDDIR)/android-base -name '*.o') \
	  $$(find $(BUILDDIR)/ziparchive -name '*.o') \
	  $(BUILDDIR)/sigchain/sigchain.o \
	  $$(find $(BUILDDIR)/generated -name '*.o') \
	  $(BUILDDIR)/asm_x86_64/quick_entrypoints_x86_64.o \
	  $(BUILDDIR)/asm_x86_64/jni_entrypoints_x86_64.o \
	  $(BUILDDIR)/asm_x86_64/memcmp16_x86_64.o \
	  $(STUBS_OBJ) \
	  $(BUILDDIR)/stubs/fault_handler_stubs.o \
	  $(BUILDDIR)/stubs/template_instantiations.o \
	  $(LDFLAGS) -lrt
	@echo "Built: $(BUILDDIR)/bin/dex2oat"
	@ls -lh $(BUILDDIR)/bin/dex2oat

ziparchive:
	@mkdir -p $(BUILDDIR)/ziparchive
	@for f in zip_archive.cc zip_archive_stream_entry.cc; do \
	  $(CXX) -std=c++17 -O2 -w -fPIC \
	    -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) \
	    -I$(AOSP)/system/core/libziparchive/include \
	    -I$(AOSP)/system/core/base/include \
	    -I$(AOSP)/system/logging/liblog/include \
	    -I$(AOSP)/system/core/liblog/include \
	    -I$(AOSP)/external/zlib \
	    -DANDROID \
	    -c $(AOSP)/system/core/libziparchive/$$f \
	    -o $(BUILDDIR)/ziparchive/$${f%.cc}.o 2>&1 && echo "OK: $$f" || echo "FAIL: $$f"; \
	done

sigchain:
	@mkdir -p $(BUILDDIR)/sigchain
	@$(CXX) -std=c++17 -O2 -w -fPIC \
	  -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) \
	  -I$(ART)/sigchainlib \
	  -I$(AOSP)/system/core/base/include \
	  -I$(AOSP)/system/logging/liblog/include \
	  -c $(ART)/sigchainlib/sigchain.cc \
	  -o $(BUILDDIR)/sigchain/sigchain.o 2>&1 && echo "OK: sigchain.cc" || echo "FAIL: sigchain.cc"

status:
	@echo "=== Build Status ==="
	@dex=$$(ls $(BUILDDIR)/libdexfile/dex/*.o 2>/dev/null | wc -l); \
	dext=$(words $(DEXFILE_OBJS)); \
	echo "libdexfile:   $$dex / $$dext"
	@art=$$(find $(BUILDDIR)/libartbase -name '*.o' 2>/dev/null | wc -l); \
	artt=$(words $(ARTBASE_OBJS)); \
	echo "libartbase:   $$art / $$artt"
	@comp=$$(find $(BUILDDIR)/compiler -name '*.o' 2>/dev/null | wc -l); \
	compt=$(words $(COMPILER_OBJS)); \
	echo "compiler:     $$comp / $$compt"
	@d2o=$$(find $(BUILDDIR)/dex2oat -name '*.o' 2>/dev/null | wc -l); \
	d2ot=$(words $(DEX2OAT_OBJS)); \
	echo "dex2oat:      $$d2o / $$d2ot"
	@rt=$$(find $(BUILDDIR)/runtime -name '*.o' 2>/dev/null | wc -l); \
	rtt=$(words $(RUNTIME_OBJS)); \
	echo "runtime:      $$rt / $$rtt"
	@dx=$$(find $(BUILDDIR)/dexlayout -name '*.o' 2>/dev/null | wc -l); \
	dxt=$(words $(DEXLAYOUT_OBJS)); \
	echo "dexlayout:    $$dx / $$dxt"
	@ef=$$(find $(BUILDDIR)/libelffile -name '*.o' 2>/dev/null | wc -l); \
	eft=$(words $(ELFFILE_OBJS)); \
	echo "libelffile:   $$ef / $$eft"
	@pf=$$(find $(BUILDDIR)/libprofile -name '*.o' 2>/dev/null | wc -l); \
	pft=$(words $(PROFILE_OBJS)); \
	echo "libprofile:   $$pf / $$pft"
	@vx=$$(find $(BUILDDIR)/vixl -name '*.o' 2>/dev/null | wc -l); \
	vxt=$(words $(VIXL_OBJS)); \
	echo "vixl:         $$vx / $$vxt"
	@ab=$$(ls $(BUILDDIR)/android-base/*.o 2>/dev/null | wc -l); \
	abt=$(words $(ABASE_OBJS)); \
	echo "android-base: $$ab / $$abt"

# Stubs object
$(STUBS_OBJ): $(STUBS)/link_stubs.cc
	@mkdir -p $(dir $@)
	@$(CXX) -std=c++17 -O2 -fPIC -w -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) -c $< -o $@ && echo "OK: link_stubs.cc" || echo "FAIL: link_stubs.cc"

# Pattern rule for ART sources
$(BUILDDIR)/%.o: $(ART)/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@ 2>&1 && echo "OK: $(notdir $<)" || { echo "FAIL: $(notdir $<)"; rm -f $@; }

# Pattern rule for VIXL sources
$(BUILDDIR)/vixl/%.o: $(AOSP)/external/vixl/%.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(VIXL_CXXFLAGS) -c $< -o $@ 2>&1 && echo "OK: $(notdir $<)" || { echo "FAIL: $(notdir $<)"; rm -f $@; }

# Pattern rule for android-base sources
$(BUILDDIR)/android-base/%.o: $(AOSP)/system/core/base/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(ABASE_CXXFLAGS) -c $< -o $@ 2>&1 && echo "OK: $(notdir $<)" || { echo "FAIL: $(notdir $<)"; rm -f $@; }

# ============ nativehelper (for dalvikvm) ============
NATIVEHELPER = $(AOSP)/libnativehelper
NATIVEHELPER_CXXFLAGS = -std=c++17 -O2 -w -fPIC -DNDEBUG \
  -stdlib=libc++ -nostdinc++ -isystem $(LIBCXX) \
  -I$(NATIVEHELPER)/include \
  -I$(NATIVEHELPER)/include_jni \
  -I$(NATIVEHELPER)/header_only_include \
  -I$(NATIVEHELPER)/platform_include \
  -I$(NATIVEHELPER) \
  -I$(AOSP)/system/core/liblog/include \
  -I$(AOSP)/system/core/base/include \
  -I$(AOSP)/prebuilts/jdk/jdk11/linux-x86/include \
  -I$(AOSP)/prebuilts/jdk/jdk11/linux-x86/include/linux

nativehelper:
	@mkdir -p $(BUILDDIR)/nativehelper
	@echo "=== nativehelper ==="
	@$(CXX) $(NATIVEHELPER_CXXFLAGS) -c $(STUBS)/nativehelper/JniInvocation_static.cpp \
	  -o $(BUILDDIR)/nativehelper/JniInvocation.o 2>&1 && echo "OK: JniInvocation_static.cpp" || echo "FAIL: JniInvocation_static.cpp"
	@$(CXX) $(NATIVEHELPER_CXXFLAGS) -c $(NATIVEHELPER)/toStringArray.cpp \
	  -o $(BUILDDIR)/nativehelper/toStringArray.o 2>&1 && echo "OK: toStringArray.cpp" || echo "FAIL: toStringArray.cpp"
	@$(CXX) $(NATIVEHELPER_CXXFLAGS) -c $(NATIVEHELPER)/JNIHelp.cpp \
	  -o $(BUILDDIR)/nativehelper/JNIHelp.o 2>&1 && echo "OK: JNIHelp.cpp" || echo "FAIL: JNIHelp.cpp"
	@$(CXX) $(NATIVEHELPER_CXXFLAGS) -c $(NATIVEHELPER)/JniConstants.cpp \
	  -o $(BUILDDIR)/nativehelper/JniConstants.o 2>&1 && echo "OK: JniConstants.cpp" || echo "FAIL: JniConstants.cpp"

# ============ dalvikvm main ============
dalvikvm-main:
	@mkdir -p $(BUILDDIR)/dalvikvm
	@echo "=== dalvikvm main ==="
	@$(CXX) $(NATIVEHELPER_CXXFLAGS) \
	  -I$(ART)/dalvikvm \
	  -c $(ART)/dalvikvm/dalvikvm.cc \
	  -o $(BUILDDIR)/dalvikvm/dalvikvm.o 2>&1 && echo "OK: dalvikvm.cc" || echo "FAIL: dalvikvm.cc"

# ============ link-runtime (dalvikvm without compiler) ============
link-runtime: all ziparchive sigchain nativehelper dalvikvm-main
	@echo "=== Linking dalvikvm (runtime only, no compiler) ==="
	@mkdir -p $(BUILDDIR)/bin
	$(HOSTLD) -o $(BUILDDIR)/bin/dalvikvm \
	  -rdynamic -Wl,--unresolved-symbols=ignore-in-object-files \
	  $(BUILDDIR)/dalvikvm/dalvikvm.o \
	  $$(find $(BUILDDIR)/nativehelper -name '*.o') \
	  $$(find $(BUILDDIR)/runtime -name '*.o') \
	  $$(find $(BUILDDIR)/libdexfile -name '*.o') \
	  $$(find $(BUILDDIR)/libartbase -name '*.o') \
	  $$(find $(BUILDDIR)/libelffile -name '*.o') \
	  $$(find $(BUILDDIR)/libprofile -name '*.o') \
	  $$(find $(BUILDDIR)/dexlayout -name '*.o') \
	  $$(find $(BUILDDIR)/vixl -name '*.o') \
	  $$(find $(BUILDDIR)/android-base -name '*.o') \
	  $$(find $(BUILDDIR)/ziparchive -name '*.o') \
	  $(BUILDDIR)/sigchain/sigchain.o \
	  $(BUILDDIR)/asm_x86_64/quick_entrypoints_x86_64.o \
	  $(BUILDDIR)/asm_x86_64/jni_entrypoints_x86_64.o \
	  $(BUILDDIR)/asm_x86_64/memcmp16_x86_64.o \
	  $(STUBS_OBJ) \
	  $(BUILDDIR)/stubs/fault_handler_stubs.o \
	  $(BUILDDIR)/stubs/template_instantiations.o \
	  $(LDFLAGS) -lrt
	@echo "Built: $(BUILDDIR)/bin/dalvikvm"
	@ls -lh $(BUILDDIR)/bin/dalvikvm

clean:
	rm -rf $(BUILDDIR)
