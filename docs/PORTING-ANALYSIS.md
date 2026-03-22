**[English](PORTING-ANALYSIS.md)** | **[中文](PORTING-ANALYSIS_CN.md)**

# ART Porting Analysis: Three Strategies for 10-50x Performance

## 1. ART Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    ART Runtime (623K lines C++)           │
│                                                           │
│  ┌───────────────────────────────────────────────────┐   │
│  │  Interpreter (2,010 lines)                         │   │
│  │  - Switch-based bytecode dispatch                  │   │
│  │  - 632 lines of intrinsics (Math, String, Array)   │   │
│  │  - 3-5x faster than Dalvik interpreter             │   │
│  └───────────────────────────────────────────────────┘   │
│                                                           │
│  ┌───────────────────────────────────────────────────┐   │
│  │  Optimizing Compiler (50K lines)                   │   │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────────────┐  │   │
│  │  │ DEX→IR  │→ │ Optimize │→ │ Code Generators  │  │   │
│  │  │ (nodes) │  │ (15 pass)│  │ ARM32/64 x86/64  │  │   │
│  │  └─────────┘  └──────────┘  └──────────────────┘  │   │
│  └───────────────────────────────────────────────────┘   │
│                                                           │
│  ┌───────────────────────────────────────────────────┐   │
│  │  Runtime Services                                  │   │
│  │  Class Linker (9,883 lines) — class loading        │   │
│  │  GC (9 collectors) — concurrent mark-sweep         │   │
│  │  JNI (12,561 lines) — native method bridge         │   │
│  │  Thread management — pthread-based                 │   │
│  └───────────────────────────────────────────────────┘   │
│                                                           │
│  ┌───────────────────────────────────────────────────┐   │
│  │  dex2oat (10K lines) — offline AOT compiler tool   │   │
│  │  Input: classes.dex → Output: native .oat file     │   │
│  └───────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### Key Dependencies (all POSIX-standard)

| Library | Purpose | Available on OHOS? |
|---------|---------|:---:|
| libz | Compression (zlib) | Yes |
| liblog | Logging | Stub or redirect |
| libbacktrace | Stack traces | Port or stub |
| libicuuc | Unicode (ICU) | Yes (OHOS has ICU) |
| libdl | Dynamic loading | Yes (musl has dlopen) |
| libartpalette | Platform abstraction | Stub (~20 methods) |
| pthreads | Threading | Yes (POSIX) |
| mmap/mprotect | Memory management | Yes (POSIX) |

**No LLVM. No Android Binder. No proprietary libraries.**

---

## 2. Strategy A: AOT Only (dex2oat)

### How It Works

```
Build time (on host x86_64):
  classes.dex → dex2oat → classes.oat (native ARM code)

Runtime (on OHOS ARM32):
  Load classes.oat → execute native code directly
  No interpreter needed (except for rare fallback)
```

### What dex2oat Does

1. Parse DEX file → extract methods
2. Build HGraph IR (intermediate representation)
3. Run 15 optimization passes:
   - Inlining (inline small methods)
   - Constant folding (evaluate constants at compile time)
   - Dead code elimination (remove unused branches)
   - Register allocation (map to CPU registers)
   - Null check elimination (prove references non-null)
   - Bounds check elimination (prove indices in range)
   - Loop optimization (unroll, strength reduce)
4. Generate native code via architecture-specific code generator
5. Output .oat file (ELF-like format with native code + metadata)

### Porting Effort

| Component | Lines | Weeks | Notes |
|-----------|------:|:-----:|-------|
| dex2oat tool | ~10K | 2 | Runs on host Linux — minimal changes |
| Compiler IR + optimizations | ~30K | 0 | Already architecture-independent |
| ARM32 code generator | ~20K | 0 | Already exists (code_generator_arm_vixl.cc) |
| .oat file loader | ~5K | 2 | ELF reader + relocation patching |
| Runtime stubs (entry/exit) | ~3K | 1 | Managed↔native transition trampolines |
| Basic GC (mark-sweep) | ~10K | 2 | Heap management with mmap |
| Class linker | ~10K | 2 | Subset — class loading + resolution |
| JNI bridge | ~5K | 1 | Adapt from Dalvik's working JNI |
| **Total** | **~93K** | **10-12** | **2-3 months** |

### Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| .oat format version coupling | Runtime must match dex2oat version | Pin both to same AOSP commit |
| ARM32 codegen bugs on OHOS | Wrong native code generated | Use existing AOSP test suite |
| mmap for GC heap | OHOS may restrict large mappings | Use smaller heap, multiple regions |
| Static linking vs shared | dex2oat expects shared libs | Build helper .so for runtime |

---

## 3. Strategy B: ART Interpreter Only

### Why 3-5x Faster Than Dalvik

| Feature | Dalvik | ART Interpreter | Speedup |
|---------|--------|-----------------|--------:|
| Dispatch | `switch(opcode)` | Computed goto (gcc extension) | 1.5-2x |
| Intrinsics | None | Math.abs, String.length, Array.length → inline | 5-50x per call |
| Field access | Hash table lookup | Direct offset (resolved at class link) | 3-5x |
| Virtual dispatch | vtable lookup every time | Inline cache (remembers last type) | 2-3x |
| String operations | Full method call | Intrinsic fast path | 5-10x |

### Porting Effort

| Component | Lines | Weeks | Notes |
|-----------|------:|:-----:|-------|
| ART interpreter | ~15K | 2 | interpreter_switch_impl-inl.h |
| Intrinsics | ~2K | 1 | interpreter_intrinsics.cc |
| Runtime core | ~20K | 3 | Thread, class linker (simplified), JNI |
| Basic GC | ~10K | 2 | Mark-sweep (reuse from Strategy A) |
| libc compatibility | ~2K | 1 | OHOS musl vs Android bionic differences |
| **Total** | **~49K** | **9** | **~2 months** |

### Why Start Here

- **Lowest risk** — no compiler, no code generation, no deoptimization
- **Drop-in replacement** — same DEX input, same JNI interface
- **Immediate benefit** — intrinsics alone give 5-50x on Math/String/Array operations
- **Foundation for A/C** — runtime core (GC, class linker, threads) is shared

---

## 4. Strategy C: Full ART (Interpreter + JIT + AOT)

### What JIT Adds Over AOT

| Capability | AOT Only | With JIT |
|-----------|:---:|:---:|
| Compile before first run | Yes | Yes |
| Compile new code at runtime | No | Yes — hot methods detected and compiled |
| Profile-guided optimization | No (static guesses) | Yes — real branch frequencies |
| Speculative inlining | Conservative | Aggressive (deoptimize if wrong) |
| Adapt to user patterns | No | Yes — re-compile with better type info |

### Why JIT Is Hard to Port

| Challenge | Why It's Hard | Effort |
|-----------|--------------|:------:|
| `mmap(PROT_EXEC)` | Need to write native code into executable memory at runtime. OHOS security may restrict this. | 2 weeks |
| Signal handlers (SIGSEGV) | ART maps address 0 as no-access, catches null dereferences as signals instead of explicit checks. OHOS signal model may differ. | 2 weeks |
| Deoptimization | When a JIT-compiled method's assumptions are invalidated (new class loaded), ART must unwind the native stack mid-execution and resume in the interpreter. This is the hardest part of JIT. | 3 weeks |
| Code cache GC | JIT-generated code lives in a special memory region. When it fills up, old code must be evicted. Needs careful coordination with the managed heap GC. | 1 week |
| Profiling | Method call counts, branch frequencies, type profiles — all collected at runtime to guide JIT decisions. Hooks into interpreter and compiled code. | 1 week |
| Thread suspension | JIT needs to stop all threads for code patching and GC. ART uses checkpoint-based suspension that's tightly integrated with the runtime. | 2 weeks |

### Porting Effort

| Component | Lines | Weeks | Notes |
|-----------|------:|:-----:|-------|
| Everything from A + B | ~93K | 10-12 | See above |
| JIT compiler integration | ~15K | 4 | jit.cc, jit_code_cache.cc |
| Signal handlers | ~3K | 2 | SIGSEGV, SIGBUS handling |
| Deoptimization | ~5K | 3 | Stack unwinding, frame reconstruction |
| Profiling | ~5K | 1 | Method counters, type profiles |
| Thread checkpoints | ~3K | 2 | Suspension, safepoints |
| **Total** | **~124K** | **22-24** | **5-6 months** |

---

## 5. Recommended Phased Approach

```
Month 1-2:  Strategy B — ART Interpreter
            └── 3-5x speedup, lowest risk, validates runtime porting
                
Month 2-4:  Strategy A — Add dex2oat AOT
            └── 10-50x speedup for pre-compiled code
            └── Use dex2oat on host, deploy .oat to device
                
Month 4-6:  Strategy C — Add JIT (optional)
            └── Dynamic optimization for code not covered by AOT
            └── Highest complexity, diminishing returns over AOT
```

### Which Strategy for Which App Type

| App Type | Strategy B (3-5x) | Strategy A (10-50x) | Strategy C (10-50x+) |
|----------|:---:|:---:|:---:|
| Calculator, Settings | Sufficient | Overkill | Not needed |
| Shopping, Social feed | Acceptable | Good | Ideal |
| Games, Maps, Camera | Too slow | Good for UI | Needed for performance |
| Real-time video/audio | Not viable | Acceptable | Required |

---

## 6. RISC-V Considerations

ART Android 11 has no RISC-V code generator. Options:

1. **Write ARM32 codegen, ignore RISC-V for now** — simplest, covers 95% of devices
2. **Use Android 14+ ART** which has initial RISC-V support — but 14 has more Android system dependencies
3. **Write a RISC-V code generator** (~20K lines, 3 months for experienced compiler developer)
4. **AOT cross-compile** — run dex2oat on x86_64 host, generate RISC-V code — needs RISC-V backend only in dex2oat, not runtime

---

## 7. Build System

ART uses Android's `soong` (Android.bp) build system. For standalone build on Linux/OHOS:

- Option 1: Convert Android.bp → CMakeLists.txt (~2 weeks)
- Option 2: Convert Android.bp → GN (used by Chrome/OHOS) (~1 week)
- Option 3: Write Makefile (simplest for initial bring-up) (~3 days)

The runtime has ~315 .cc files. A flat Makefile listing all sources works for bring-up.
