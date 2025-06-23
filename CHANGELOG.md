# 1.0.2-alpha
- Implemented module for procfs
- Fixed unreadable symbolic link error

# 1.0.1-alpha
- Added UnicornAFL fuzzing mode
- Added udbserver
- Added baremetal support
- ArionGroup class (preparing IPC)
- Improved syscall logging
- Fixed submodules compilation issue
- Implemented Google Test
- Removed DEV compilation mode
- Added architecture rootfs and static toolchains build/download
- Added CMake package
- Fixed dependency issues when linking in DEV mode
- Added config for Arion instance
- Implemented some syscalls
- Run from/to address
- Signals rework
- Threads rework
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
