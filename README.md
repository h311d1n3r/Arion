# Arion

<div align="center"><img src="./res/img/logo.png" alt="Arion Logo" width="300"></div>

## A high-performance C++ framework for emulating executable binaries  
**Arion** is a library that aims to emulate various **executable formats** (ELF, PE, Mach-O...) coming from **different platforms** (Linux, Windows, macOS...) and with **different CPU architectures** (x86, ARM, MIPS...).  
Based on [**Unicorn**](https://github.com/unicorn-engine/unicorn) and written in **C++**, it should allow fast emulation especially for **fuzzing purposes**.  
Inspired by [**Qiling**](https://github.com/qilingframework/qiling), Arion in its current form is not intended to replace this awesome Python library, but to complement it **with higher performance**.  

## Current state of the project
### Warning
**This project is still in alpha development. It can be unstable and/or lead to undesired behaviors so you may want to deploy it in a containerized environment.**  
### Features
Arion currently implements the following features :  
- **Emulating Linux ELFs for x86, x86-64, ARM, ARM64**  
- **Emulating more than 120 syscalls**  
- **Fork handling**  
- **Multithreading handling (unstable)**  
- **Saving / restoring context**  
- **Hooking the target with ~20 functions**  
- **Memory reading / writing**  
- **File system management**  
- **Network sockets management**  

## Table of contents
[Installation](#install)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Download a release](#install_release)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Build the library with Docker](#install_build_docker)  
[Performance comparison](#perfs)  
[How to use ?](#how)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Examples](#how_examples)  
[Contributing](#contributing)

<a name="install"/>

## Installation

<a name="install_release"/>

### Download a release
Check the [Releases](https://github.com/h311d1n3r/Arion/releases/) tab on the Github project and download the latest one.  

<a name="install_build_docker"/>

### Build the library with Docker
1. Clone the repository `git clone https://github.com/h311d1n3r/Arion.git && cd Arion`.
2. Check the available Dockerfiles under `Arion/docker`.  
3. Build the docker image of your choice `./scripts/docker_build.sh {OS}{OS_VERSION} {BUILD_VERSION}`.  
4. You can build against **Arion** library from inside the docker or extract it on your host.  

<a name="install_build_host"/>

### Build the tool on host  
1. Clone the repository `git clone https://github.com/h311d1n3r/Arion.git && cd Arion`.
2. Initialize git dependencies : `git submodule update --init`  
3. Create the build directory `mkdir build && cd build`.  
4. Run CMake to configure the project `cmake ..`.
5. Run make to compile the project `make -j4`.
6. Run make install to deploy the project `sudo make install`.  

<a name="perfs"/>

## Performance comparison
Since Arion is entirely written in C++, it has a much lower execution time than Qiling because of its to-and-fro in the Python context.  
The next two graphs have been realized with the same program, run in the same context with both Arion and Qiling.
In the first graph, the variable is the amount of syscalls executed by the target wheras in the second it is the amount of basic blocks hit, when all basic blocks are hooked.  
<div align="center"><img src="./res/img/arion_ql_syscalls.png" alt="Arion Logo" width="900"></div>
<div align="center"><img src="./res/img/arion_ql_bbs.png" alt="Arion Logo" width="900"></div>

<a name="how"/>

## How to use ?
**A wiki and a documentation are to come**. For now, you can rely on the examples and visit the headers in `include` directory to learn more about what you can do with **Arion**.  

<a name="how_examples"/>

### Examples
You can find examples inside the `examples` directory. These examples are by no means exhaustive, but they will teach you how to deal with **Arion**.  

<a name="contributing"/>

## Contributing

Feel free to contribute to the project by implementing new features on the `dev` branch.  
