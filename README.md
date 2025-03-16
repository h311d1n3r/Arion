# Arion

<div align="center"><img src="./res/img/logo.png" alt="Arion Logo" width="300"></div>

## A high-performance C++ framework for emulating executable binaries  
Arion is a library that aims to emulate various executable formats (ELF, PE, Mach-O...) coming from different platforms (Linux, Windows, macOS...) and with different CPU architectures (x86, ARM, MIPS...).  
Based on Unicorn and written in C++, it should allow fast emulation especially for fuzzing purposes.  
Inspired by Qiling, Arion in its current form is not intended to replace this awesome Python library, but to complement it with higher performance.  

## Current state of the project
### Warning
**This project is still in alpha development. It can be unstable and/or lead to undesired behaviors so you may want to deploy it in a containerized environment.**  
### Features
Arion currently implements the following features :  
- Emulating Linux ELFs for x86, x86-64, ARM, ARM64  
- Emulating more than 120 syscalls  
- Fork handling  
- Multithreading handling (unstable)  
- Saving / restoring context  
- Hooking the target with ~20 functions  
- Memory reading / writing  
- File system management  
- Network sockets management  

## Table of contents
[Installation](#install)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Download a release](#install_release)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Build the library with Docker](#install_build_docker)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Build the library on host](#install_build_host)  
[How to use ?](#how)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Example](#how_example)  
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

<a name="how"/>

## How to use ?
A wiki and a documentation are to come. For now, you can rely on the following example and visit the headers in `include` directory to learn more about what you can do with Arion.  

<a name="how_example"/>

### Example
Here is a simple example of emulating `/bin/ls` and playing around with it.  

```C++
#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>

using namespace arion;

void instr_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    std::vector<BYTE> read_data = arion->mem->read(addr, sz);
    cs_insn *insn;
    size_t count = cs_disasm(*arion->abi->curr_cs(), (const uint8_t *)read_data.data(), sz, addr, 0, &insn);
    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            std::cout << "PID: 0x" << std::hex << +arion->get_pid() << " TID: 0x" << std::hex
                      << +arion->threads->get_running_tid() << " - " << insn[i].address << ":\t" << insn[i].mnemonic
                      << "\t" << insn[i].op_str << std::endl;
        }

        cs_free(insn, count);
    }
    else
    {
        std::cerr << "Failed to disassemble code." << std::endl;
    }
}

void block_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    ADDR rsp = arion->abi->read_reg<RVAL64>("RSP");
    uint64_t curr_stack_val = arion->mem->read_val(rsp, sizeof(uint64_t));
    std::cout << "New basic block at 0x" << std::hex << +addr << " RSP : 0x" << std::hex << +rsp << " [RSP] : 0x" << std::hex
              << +curr_stack_val << std::endl;
}

void execve_hook(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> new_inst, void *user_data)
{
    std::cout << "EXECVE !" << std::endl;
    std::cout << "PID : 0x" << std::hex << +new_inst->get_pid() << std::endl;
    std::cout << "PROCESS IMAGE : " << new_inst->get_program_args().at(0) << std::endl;
    new_inst->hooks->hook_code(instr_hook);
}
void fork_hook(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> child, void *user_data)
{
    std::cout << "FORK !" << std::endl;
    std::cout << "PARENT PID : 0x" << std::hex << +arion->get_pid() << std::endl;
    std::cout << "CHILD PID : 0x" << std::hex << +child->get_pid() << std::endl;
    child->hooks->hook_code(instr_hook);
    child->hooks->hook_execve(execve_hook);
}

int main()
{
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    std::shared_ptr<Arion> arion = Arion::new_instance({"/bin/ls"}, "/", {}, "/", ARION_LOG_LEVEL::OFF);
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->hooks->hook_code(instr_hook);
    arion->hooks->hook_block(block_hook);
    // Won't be called for /bin/ls
    arion->hooks->hook_execve(execve_hook);
    arion->hooks->hook_fork(fork_hook);
    arion->run();
    return 0;
}
```

<a name="contributing"/>

## Contributing

Feel free to contribute to the project by implementing new features on the `dev` branch.  
