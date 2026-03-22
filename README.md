**[English](README.md)** | **[中文](README_CN.md)**

# ART Universal: Android Runtime for Modern Platforms

[![License](https://img.shields.io/badge/license-Apache_2.0-blue)]()
[![Source](https://img.shields.io/badge/source-AOSP_Android_11-green)]()
[![Lines](https://img.shields.io/badge/code-623K_lines_C++-orange)]()

Port the Android Runtime (ART) to run on any modern platform — Linux x86_64, OHOS ARM32/64, Ubuntu ARM64, and future RISC-V. **10-50x performance improvement** over Dalvik interpreter.

## Why ART Universal

The [Westlake engine](https://github.com/A2OH/westlake) currently uses KitKat Dalvik VM, which interprets every bytecode instruction at ~500ns each. ART compiles DEX to native machine code, achieving ~1-5ns per operation.

| Metric | Dalvik (current) | ART (target) | Speedup |
|--------|:---:|:---:|:---:|
| getMeasuredWidth() | ~500ns | ~2ns | 250x |
| 100-view layout frame | ~50ms | ~1ms | 50x |
| ListView scroll FPS | ~20fps | ~120fps | 6x |
| GC pause | 10-50ms (STW) | ~1ms (concurrent) | 10-50x |

## ART Source Facts

- **Fully open source** — Apache 2.0 from AOSP
- **623,153 lines** of C++
- **No LLVM dependency** — has its own optimizing compiler backend
- **Built-in code generators** for ARM32 (VIXL), ARM64, x86, x86_64
- **Linux host build supported** — `host_supported: true` in Android.bp
- **Only 2 Android system references** in entire runtime.cc (trivially stubbed)
- **POSIX-only runtime** — pthreads, mmap, file I/O — works on OHOS

```
aosp/art/                          623,153 lines
├── runtime/          (315 .cc)    VM core: class loading, GC, threads, interpreter
├── compiler/         (159 .cc)    Optimizing compiler + code generators
├── dex2oat/          (34 .cc)     Ahead-of-time compilation tool
├── libdexfile/       (34 .cc)     DEX file parser
└── test/                          Extensive test suite
```

## Three Porting Strategies

See **[docs/PORTING-ANALYSIS.md](docs/PORTING-ANALYSIS.md)** for the full technical deep dive.

| Strategy | Speedup | Effort | Risk | Phase |
|----------|:-------:|:------:|:----:|:-----:|
| **B: ART Interpreter** | 3-5x | 1-2 months | Low | Month 1-2 |
| **A: dex2oat AOT** | 10-50x | 2-3 months | Medium | Month 2-4 |
| **C: Full ART (JIT)** | 10-50x | 4-6 months | High | Month 4-6 |

**Recommended:** Start with Strategy B (quick 3-5x win), then add Strategy A (10-50x).

## Target Platforms

| Platform | Code Generator | Status |
|----------|:-:|:---:|
| x86_64 Linux | code_generator_x86_64.cc | AOSP already supports |
| ARM32 OHOS | code_generator_arm_vixl.cc | Target for Westlake |
| ARM64 OHOS/Ubuntu | code_generator_arm64.cc | Target |
| RISC-V | New codegen needed | Future (Android 14+ has initial support) |

## Dependencies

| Project | Repository | Relationship |
|---------|-----------|-------------|
| **Westlake** | [A2OH/westlake](https://github.com/A2OH/westlake) | Consumer — uses ART to execute DEX |
| **Dalvik Universal** | [A2OH/dalvik-universal](https://github.com/A2OH/dalvik-universal) | Predecessor — ART replaces Dalvik |
| **OpenHarmony WSL** | [A2OH/openharmony-wsl](https://github.com/A2OH/openharmony-wsl) | Platform — ART runs on OHOS |

## Getting Started

```bash
# Clone ART source
git clone https://android.googlesource.com/platform/art -b android-11.0.0_r1 ~/art

# Or use from existing AOSP checkout
ls ~/aosp-android-11/art/

# Key files to review first
art/runtime/runtime.cc                                  # VM initialization (2 Android refs)
art/runtime/interpreter/interpreter_switch_impl-inl.h   # Bytecode loop (2010 lines)
art/runtime/interpreter/interpreter_intrinsics.cc        # Intrinsics (632 lines)
art/compiler/optimizing/code_generator_arm_vixl.cc       # ARM32 codegen
art/dex2oat/dex2oat.cc                                  # AOT compiler entry point
```

## License

Apache 2.0 — same as AOSP.
