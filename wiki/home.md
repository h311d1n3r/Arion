\mainpage Arion

\image html logo.png "Arion Logo" width=300cm

## Overview

**Arion** is a library that aims to emulate various **executable formats** (ELF, PE, Mach-O...) coming from **different platforms** (Linux, Windows, macOS...) and with **different CPU architectures** (x86, ARM, MIPS...).  
Based on [**Unicorn**](https://github.com/unicorn-engine/unicorn) and written in **C++**, it allows fast emulation especially for **fuzzing purposes**.  
Inspired by [**Qiling**](https://github.com/qilingframework/qiling), Arion in its current form is not intended to replace this Python library, but to complement it **with higher performance**.  

## Features

- **Emulating Linux ELFs for x86, x86-64, ARM, ARM64**  
- **Snapshot fuzzing with UnicornAFL**  
- **Emulating more than 120 syscalls**  
- **Fork handling**  
- **Multithreading handling**  
- **Saving / restoring context**  
- **GDB debugging with udbserver**
- **Hooking the target with ~20 functions**  
- **Memory reading / writing**  
- **File system management**  
- **Network sockets management**  
- **Coredumps support**

## Getting Started

See the [`arion` namespace](namespaces.html) for the core API.
