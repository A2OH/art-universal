**[English](README.md)** | **[中文](README_CN.md)**

# ART Universal: Android Runtime for Modern Platforms

[![License](https://img.shields.io/badge/license-Apache_2.0-blue)]()
[![Source](https://img.shields.io/badge/source-AOSP_Android_11-green)]()
[![Lines](https://img.shields.io/badge/code-623K_lines_C++-orange)]()
[![Status](https://img.shields.io/badge/status-All_3_strategies_COMPLETE-brightgreen)]()

Port the Android Runtime (ART) to run on any modern platform — Linux x86_64, OHOS ARM64, and future targets. **13-56x measured speedup** over Dalvik interpreter.

## Real Benchmark: Dalvik vs ART (Measured)

Same machine, same DEX bytecode, same TinyBench program:

| Benchmark | Dalvik KitKat (ms) | ART AOT (ms) | Speedup |
|-----------|---:|---:|---:|
| Method calls (10M) | 129 | 3 | **43x** |
| Field access (10M) | 107 | 2 | **54x** |
| Fibonacci(40) recursive | 7,483 | 133 | **56x** |
| Tight loop sum (100M) | 939 | 33 | **28x** |
| Object allocation (1M) | 116 | 9 | **13x** |
| **Total** | **8,774** | **180** | **49x** |

**Test methodology:** Both VMs on x86-64 Linux, identical DEX bytecode. Dalvik = KitKat portable interpreter. ART AOT = dex2oat `--compiler-filter=speed` boot image. JIT achieves same speed for hot methods via runtime compilation.

## What's Built

```
art-universal-build/
├── build/bin/dex2oat          17MB   AOT compiler (421 source files)
├── build/bin/dalvikvm         13MB   x86-64 runtime (with JIT support)
├── build/lib/libart-compiler.so 9.3MB JIT compiler library
├── build/lib/libicu_jni.so           ICU charset stubs (20 methods)
├── build/lib/libjavacore.so          POSIX I/O stubs (46 methods)
├── build/lib/libopenjdk.so           OpenJDK stubs (57 methods)
├── build-ohos-arm64/bin/dalvikvm 7.5MB  OHOS ARM64 static binary
├── Makefile                          x86-64 build (AOSP clang 11)
├── Makefile.ohos-arm64               OHOS ARM64 cross-compile (OHOS clang 15)
├── stubs/                            Symbol stubs, JNI stubs, compat headers
└── patches/                          ART source patches (verifier, thread, compiler)
```

## All Three Strategies — COMPLETE

| Strategy | Speedup | Status | Evidence |
|----------|:-------:|:------:|----------|
| **A: dex2oat AOT** | 13-56x | **DONE** | Boot image with 8.7MB compiled native code |
| **B: C++ Interpreter** | ~1-2x | **DONE** | HelloArt exit code 0 on x86-64 + ARM64 QEMU |
| **C: JIT** | Same as AOT | **DONE** | `renderLoop()` compiled at runtime in 741μs |

### Strategy A: AOT Compilation
```bash
# Compile DEX to native code (offline)
./build/bin/dex2oat --dex-file=app.dex --oat-file=app.oat \
  --boot-image=boot.art --instruction-set=x86_64 --compiler-filter=speed
```
Produces .oat ELF shared objects with native machine code. 13-56x speedup measured.

### Strategy B: C++ Switch Interpreter
```bash
# Run DEX bytecode via interpreter (no compilation needed)
./build/bin/dalvikvm -Xbootclasspath:core-oj.jar:core-libart.jar \
  -Xverify:none -classpath app.dex com.example.Main
```
Works on all platforms. Fallback when AOT code isn't available.

### Strategy C: JIT Runtime Compilation
```bash
# Run with JIT enabled — hot methods compiled at runtime
./build/bin/dalvikvm -Xusejit:true -Xbootclasspath:... -classpath app.dex Main
```
JIT profiles running code, compiles hot methods (threshold: 10,240 invocations). MockDonalds `renderLoop()` JIT-compiled to 824 bytes native code in 741μs.

```
JIT created with initial_capacity=64KB, max_capacity=64MB
Start profiling MockDonaldsApp.renderLoop()
Compiling method MockDonaldsApp.renderLoop() took 740.958us
JIT added MockDonaldsApp.renderLoop() ccache_size=824B
```

## MockDonalds End-to-End on ART

Full Android Activity lifecycle running at 60fps on x86-64:

```
[MockDonaldsApp] Starting on OHOS + ART ...
[MockDonaldsApp] OHBridge native: LOADED (169 methods)
[MockDonaldsApp] MiniServer initialized
[D] MiniActivityManager: startActivity: com.example.mockdonalds.MenuActivity
[D] MiniActivityManager:   performCreate → performStart → performResume
[MockDonaldsApp] MenuActivity launched
[MockDonaldsApp] Creating surface 480x800
[MockDonaldsApp] Initial frame rendered
[MockDonaldsApp] Frame 600 activity=MenuActivity
```

## Build

### x86-64 (host)
```bash
# Requires: AOSP 11 source tree at ~/aosp-android-11/
make -j$(nproc) all          # Compile 421 source files
make link                     # Link dex2oat (17MB)
make link-runtime             # Link dalvikvm (13MB, with -rdynamic for JIT)
```

### OHOS ARM64 (cross-compile)
```bash
# Requires: OHOS clang 15 + aarch64 sysroot
make -f Makefile.ohos-arm64 -j$(nproc) link-runtime
# Produces: build-ohos-arm64/bin/dalvikvm (7.5MB static ARM64)
```

### Boot image
```bash
./build/bin/dex2oat \
  --dex-file=core-oj.jar --dex-file=core-libart.jar \
  --dex-file=core-icu4j.jar --dex-file=aosp-shim.dex \
  --oat-file=boot.oat --image=boot.art \
  --instruction-set=arm64 --compiler-filter=speed \
  --base=0x70000000 --runtime-arg -Xverify:none -j4
```

## Key Bugs Fixed During Port

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| IfTable offset 0 vs 8 | AOSP clang 11 inlining bug in verifier | Recompile verifier files with `-O1` |
| Null class in FromClass | Unresolvable types during boot image | Null guard returning Conflict type |
| Re-entrant VerifyClass deadlock | ThrowVerifyError → EnsureInitialized → VerifyClass(Object) loop | Skip EnsureInitialized during AOT |
| artFindNativeMethod stub | JNI entrypoints not compiled | Added `entrypoints/jni/*.cc` to Makefile |
| DexCache 128-bit atomics | x86 uses cmpxchg16b, ARM64 uses ldxp/stlxp | Platform-specific stubs |
| Static build dlsym | Fake handles can't dlsym | `dlopen(NULL)` returns main program handle |

## 123 JNI Native Method Stubs

| Library | Methods | Purpose |
|---------|:-------:|---------|
| libicu_jni.so | 20 | ICU charset conversion (UTF-8, ASCII, ISO-8859-1) |
| libjavacore.so | 46 | POSIX I/O (open/read/write/close), getpwuid, environ, sysconf |
| libopenjdk.so | 57 | System.nanoTime, Runtime, FileDescriptor, NIO, UnixFileSystem, ZipFile, Math |

## Target Platforms

| Platform | Status | Binary |
|----------|:------:|--------|
| x86_64 Linux | **Working** | `build/bin/dalvikvm` (13MB, dynamic) |
| OHOS ARM64 | **Working** | `build-ohos-arm64/bin/dalvikvm` (7.5MB, static) |
| ARM64 QEMU | **Working** | HelloArt exit code 0, MockDonalds launches |
| OHOS ARM32 | Future | Assembly entry points available in AOSP |
| RISC-V | Future | Android 14+ has initial support |

## Repository Map

| Repo | Purpose |
|------|---------|
| **[A2OH/art-universal](https://github.com/A2OH/art-universal)** | This repo — ART build system, stubs, patches |
| [A2OH/westlake](https://github.com/A2OH/westlake) | Integration — AOSP framework + OHBridge + apps |
| [A2OH/dalvik-universal](https://github.com/A2OH/dalvik-universal) | Dalvik VM baseline (KitKat portable interpreter) |
| [A2OH/openharmony-arm64](https://github.com/A2OH/openharmony-arm64) | OHOS ARM64 QEMU environment |
| [A2OH/openharmony-wsl](https://github.com/A2OH/openharmony-wsl) | OHOS ARM32 QEMU environment |

## License

Apache 2.0 — same as AOSP.
