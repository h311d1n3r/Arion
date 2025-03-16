#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>

using namespace arion;

void insn_hook(std::shared_ptr<Arion> arion, void *user_data)
{
    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    ADDR pc_val = arion->abi->read_reg<RVAL64>(pc_reg); // You could just do read_reg<RVAL64>("rip");
    ADDR rax_val = arion->abi->read_reg<RVAL64>("RAX");
    ADDR rbx_val = arion->abi->read_reg<RVAL64>("RBX");
    ADDR rcx_val = arion->abi->read_reg<RVAL64>("RCX");
    ADDR rdx_val = arion->abi->read_reg<RVAL64>("RDX");
    std::cout << "CPUID AT : 0x" << std::hex << +pc_val << " RAX : 0x" << std::hex << +rax_val << " RBX : 0x"
              << std::hex << +rax_val << " RCX : 0x" << std::hex << +rax_val << " RDX : 0x" << std::hex << +rax_val
              << std::endl;
    // You could write a register with arion->abi->write_reg<RVAL64>("rax", 1);
}

int main()
{
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    std::shared_ptr<Arion> arion = Arion::new_instance({"/bin/ls"}, "/", {}, "/", ARION_LOG_LEVEL::DEBUG);
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->hooks->hook_insn(insn_hook, UC_X86_INS_CPUID);
    arion->run();
    return 0;
}
