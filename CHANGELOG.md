# 1.0.1-alpha
- Added udbserver
- Fixed submodules compilation issue
- Removed DEV mode
- Improved syscall logging
- Added architecture rootfs and static toolchains build/download
- Implemented Google Test
- Added CMake package
- Fixed dependency issues when linking in DEV mode
- Added config for Arion instance
- Implemented some syscalls
- Added UnicornAFL fuzzing mode
- Run from/to address
- Signals rework
- Threads rework
- ArionGroup class (preparing IPC)
- Bumped CMake requirements
- Added CHANGELOG.md and VERSION

# 1.0.0-alpha
- Emulating Linux ELFs for x86, x86-64, ARM, ARM64
- Emulating more than 120 syscalls
- Fork handling
- Multithreading handling (unstable)
- Saving / restoring context
- Hooking the target with ~20 functions
- Memory reading / writing
- File system management
- Network sockets management
