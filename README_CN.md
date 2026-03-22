**[English](README.md)** | **[中文](README_CN.md)**

# ART Universal: 面向现代平台的Android运行时

[![License](https://img.shields.io/badge/license-Apache_2.0-blue)]()
[![Source](https://img.shields.io/badge/source-AOSP_Android_11-green)]()
[![Lines](https://img.shields.io/badge/code-623K_lines_C++-orange)]()

将Android运行时(ART)移植到任何现代平台——Linux x86_64、OHOS ARM32/64、Ubuntu ARM64以及未来的RISC-V。相比Dalvik解释器实现**10-50倍性能提升**。

## 为什么需要ART Universal

[Westlake引擎](https://github.com/A2OH/westlake)目前使用KitKat Dalvik VM，逐条解释字节码指令，每条约500ns。ART将DEX编译为原生机器码，每次操作仅约1-5ns。

| 指标 | Dalvik（当前） | ART（目标） | 加速 |
|------|:---:|:---:|:---:|
| getMeasuredWidth() | ~500ns | ~2ns | 250倍 |
| 100个View布局帧 | ~50ms | ~1ms | 50倍 |
| ListView滚动FPS | ~20fps | ~120fps | 6倍 |
| GC暂停 | 10-50ms（全停顿） | ~1ms（并发） | 10-50倍 |

## ART源码特点

- **完全开源** — Apache 2.0，来自AOSP
- **623,153行** C++代码
- **无LLVM依赖** — 自带优化编译器后端
- **内置代码生成器** — ARM32 (VIXL)、ARM64、x86、x86_64
- **支持Linux主机构建** — Android.bp中`host_supported: true`
- **仅2处Android系统引用** — 极易桩化
- **纯POSIX运行时** — pthreads、mmap、文件I/O — 在OHOS上可用

## 三种移植策略

详见 **[docs/PORTING-ANALYSIS_CN.md](docs/PORTING-ANALYSIS_CN.md)**

| 策略 | 加速 | 工作量 | 风险 | 阶段 |
|------|:----:|:------:|:----:|:----:|
| **B：ART解释器** | 3-5倍 | 1-2个月 | 低 | 第1-2月 |
| **A：dex2oat AOT** | 10-50倍 | 2-3个月 | 中 | 第2-4月 |
| **C：完整ART（JIT）** | 10-50倍 | 4-6个月 | 高 | 第4-6月 |

**推荐：** 先用策略B（快速3-5倍提升），再加策略A（10-50倍）。

## 目标平台

| 平台 | 代码生成器 | 状态 |
|------|:-:|:---:|
| x86_64 Linux | code_generator_x86_64.cc | AOSP已支持 |
| ARM32 OHOS | code_generator_arm_vixl.cc | Westlake目标 |
| ARM64 OHOS/Ubuntu | code_generator_arm64.cc | 目标 |
| RISC-V | 需新代码生成器 | 未来（Android 14+有初步支持） |

## 关联项目

| 项目 | 仓库 | 关系 |
|------|------|------|
| **Westlake** | [A2OH/westlake](https://github.com/A2OH/westlake) | 使用ART执行DEX |
| **Dalvik Universal** | [A2OH/dalvik-universal](https://github.com/A2OH/dalvik-universal) | 前身——ART替代Dalvik |
| **OpenHarmony WSL** | [A2OH/openharmony-wsl](https://github.com/A2OH/openharmony-wsl) | 平台——ART运行于OHOS |

## 许可证

Apache 2.0 — 与AOSP相同。
