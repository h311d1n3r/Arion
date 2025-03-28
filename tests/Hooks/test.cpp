#include <arion/arion.hpp>
#include <filesystem>
#include <iostream>
#include <cstdint>

std::map<std::string, bool> hits = {
    {"BKPT", false},
    {"INSN", false},
    {"CODE", false},
    {"BLCK", false},
};

void intr_hook(std::shared_ptr<Arion> arion, uint32_t val, void *user_data) {
    if(val == CPU_INTR::BREAKPOINT)
        hits["BKPT"] = true;
}

void insn_hook(std::shared_ptr<Arion> arion, void *user_data) {
    hits["INSN"] = true;
}

void code_hook(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data) {
    hits["CODE"] = true;
}

void block_hook(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data) {
    hits["BLCK"] = true;
}

int main() {
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<Arion> arion = Arion::new_instance(std::vector<std::string>{"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion->hooks->hook_intr(intr_hook);
    arion->hooks->hook_insn(insn_hook, UC_X86_INS_SYSCALL);
    arion->hooks->hook_code(code_hook);
    arion->hooks->hook_block(block_hook);
    try {
        arion->run();
    } catch(std::exception e) {} // Breakpoint will lead to exception
    for(auto hit_it : hits)
        std::cout << hit_it.first << ": " << (hit_it.second ? "TRUE" : "FALSE") << std::endl;
}