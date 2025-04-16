# 1.0.2-alpha
- Added CMake package
- Reduced compilation time
- Fixed dependency issues when linking in DEV mode
- Added config for Arion instance
- Implemented some syscalls
- Added UnicornAFL fuzzing mode
- Run from/to address
- ArionGroup class (preparing IPC)

# 1.0.1-alpha
- Bumped CMake requirements
- Added CHANGELOG.md/VERSION.txt

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
