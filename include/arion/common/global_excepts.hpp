#ifndef ARION_GLOBAL_EXCEPTS_HPP
#define ARION_GLOBAL_EXCEPTS_HPP

#include <arion/capstone/capstone.h>
#include <arion/common/global_defs.hpp>
#include <arion/keystone/keystone.h>
#include <arion/unicorn/unicorn.h>
#include <arion/utils/convert_utils.hpp>
#include <cstdint>
#include <dlfcn.h>
#include <execinfo.h>
#include <string.h>
#include <string>

#define EXCEPTION_MAX_FRAMES 64

class ArionException : public std::exception
{
  private:
    std::string message;

  public:
    ArionException(std::string msg) : message(msg)
    {
        void *buffer[EXCEPTION_MAX_FRAMES];
        int frame_count = backtrace(buffer, EXCEPTION_MAX_FRAMES);
        std::ostringstream ss;
        ss << msg;
        ss << std::endl << std::endl << "========== BACKTRACE ==========" << std::endl << std::endl;
        for (int i = 0; i < frame_count; ++i)
        {
            Dl_info info;
            if (dladdr(buffer[i], &info))
            {
                uintptr_t offset = 0;
                if (info.dli_saddr)
                    offset = reinterpret_cast<uintptr_t>(buffer[i]) - reinterpret_cast<uintptr_t>(info.dli_saddr);
                ss << std::setw(3) << i << ": "
                   << (info.dli_sname ? demangle_cxx_symbol(std::string(info.dli_sname)) : "??") << " + "
                   << int_to_hex<uintptr_t>(offset) << " (" << buffer[i] << ") at "
                   << (info.dli_fname ? info.dli_fname : "??") << std::endl;
            }
            else
            {
                ss << std::setw(3) << i << ": [unknown] (" << buffer[i] << ")\n";
            }
        }
        ss << std::endl << "===============================" << std::endl << std::endl;
        this->message = ss.str();
    }

    const char *what() const noexcept override
    {
        return message.c_str();
    }
};

class InvalidArgumentException : public ArionException
{
  public:
    explicit InvalidArgumentException(std::string err)
        : ArionException(std::string("Invalid argument : \"") + err + std::string("\".")) {};
};

class ExpiredWeakPtrException : public ArionException
{
  public:
    explicit ExpiredWeakPtrException(std::string weak_ptr_name)
        : ArionException(std::string("Attempting to use an expired weak_ptr : \"") + weak_ptr_name +
                         std::string("\".")) {};
};

class FileNotFoundException : public ArionException
{
  public:
    explicit FileNotFoundException(std::string file_path)
        : ArionException(std::string("File could not be found \"") + file_path + std::string("\".")) {};
};

class FileOpenException : public ArionException
{
  public:
    explicit FileOpenException(std::string file_path)
        : ArionException(std::string("An error occurred while opening file \"") + file_path + std::string("\".")) {};
};

class FileTooSmallException : public ArionException
{
  public:
    explicit FileTooSmallException(std::string file_path, size_t file_sz, size_t min_file_sz)
        : ArionException(std::string("File size of \"") + file_path + std::string("\" is ") +
                         int_to_hex<size_t>(file_sz) + std::string(" when it should be at least ") +
                         int_to_hex<size_t>(min_file_sz) + std::string(".")) {};
};

class FileNotInFsException : public ArionException
{
  public:
    explicit FileNotInFsException(std::string fs, std::string file_path)
        : ArionException(std::string("File \"") + file_path + std::string("\" is not in file system \"") + fs +
                         std::string("\".")) {};
};

class ConfigKeyNotFoundException : public ArionException
{
  public:
    explicit ConfigKeyNotFoundException(std::string key)
        : ArionException(std::string("Configuration doesn't have a \"") + key + std::string("\" key.")) {};
};

class ConfigWrongTypeAccessException : public ArionException
{
  public:
    explicit ConfigWrongTypeAccessException(std::string key)
        : ArionException(std::string("Configuration key \"") + key +
                         std::string("\" was accessed with a wrong type.")) {};
};

class WrongContextFileMagicException : public ArionException
{
  public:
    explicit WrongContextFileMagicException(std::string file_path)
        : ArionException(std::string("File \"") + file_path +
                         std::string("\" does not start with required magic sequence for a context file.")) {};
};

class NewerContextFileVersionException : public ArionException
{
  public:
    explicit NewerContextFileVersionException(std::string file_path)
        : ArionException(
              std::string("Context file \"") + file_path +
              std::string("\" was written with a newer version of Arion. Consider updating Arion to use it.")) {};
};

class PathTooLongException : public ArionException
{
  public:
    explicit PathTooLongException(std::string file_path)
        : ArionException(std::string("File path \"") + file_path + std::string("\" is too long.")) {};
};

class UnknownLinkageTypeException : public ArionException
{
  public:
    explicit UnknownLinkageTypeException(std::string exe_path)
        : ArionException(std::string("Coudln't identify linkage type of \"") + exe_path + std::string("\".")) {};
};

class InvalidSyscallNoException : public ArionException
{
  public:
    explicit InvalidSyscallNoException(uint64_t syscall_no)
        : ArionException(std::string("Invalid syscall number : \"") + int_to_hex(syscall_no) + std::string("\".")) {};
};

class InvalidSyscallNameException : public ArionException
{
  public:
    explicit InvalidSyscallNameException(std::string &name)
        : ArionException(std::string("Invalid syscall name : \"") + name + std::string("\".")) {};
};

class BadLinkageTypeException : public ArionException
{
  public:
    explicit BadLinkageTypeException(std::string exe_path)
        : ArionException(std::string("Wrong linkage type for \"") + exe_path + std::string("\".")) {};
};

class NoCoredumpCurrentThreadException : public ArionException
{
  public:
    explicit NoCoredumpCurrentThreadException()
        : ArionException(std::string("Can't analyze coredump note because it does not refer to a thread.")) {};
};

class NoSegmentAtAddrException : public ArionException
{
  public:
    explicit NoSegmentAtAddrException(arion::ADDR addr)
        : ArionException(std::string("No memory segment is mapped at address ") + int_to_hex(addr) + std::string(".")) {
          };
};

class NoSegmentWithInfoException : public ArionException
{
  public:
    explicit NoSegmentWithInfoException(std::string info)
        : ArionException(std::string("No memory segment has info ") + info + std::string(".")) {};
};

class SegmentNotMappedException : public ArionException
{
  public:
    explicit SegmentNotMappedException(arion::ADDR start_addr, arion::ADDR end_addr)
        : ArionException(std::string("Segment [") + int_to_hex(start_addr) + std::string(":") + int_to_hex(end_addr) +
                         std::string("] is not mapped in memory.")) {};
};

class MemAlreadyMappedException : public ArionException
{
  public:
    explicit MemAlreadyMappedException(arion::ADDR start_addr, size_t sz)
        : ArionException(std::string("An error occurred while attempting to map memory at address ") +
                         int_to_hex<arion::ADDR>(start_addr) + std::string(" with size ") + int_to_hex<ssize_t>(sz) +
                         std::string(".")) {};
};

class CantUnmapOutsideSegmentException : public ArionException
{
  public:
    explicit CantUnmapOutsideSegmentException(arion::ADDR seg_start, arion::ADDR seg_end, arion::ADDR unmap_start,
                                              arion::ADDR unmap_end)
        : ArionException(std::string("Segment [") + int_to_hex(seg_start) + std::string(":") + int_to_hex(seg_end) +
                         std::string("] is not between unmapping bounds [") + int_to_hex(unmap_start) +
                         std::string(":") + int_to_hex(unmap_end) + std::string("].")) {};
};

class UnsupportedCpuArchException : public ArionException
{
  public:
    explicit UnsupportedCpuArchException()
        : ArionException("This CPU architecture in not supported for the moment.") {};
};

class UnsupportedHostCpuArchException : public ArionException
{
  public:
    explicit UnsupportedHostCpuArchException()
        : ArionException("Your host CPU architecture in not supported for the moment.") {};
};

class TooManyThreadsException : public ArionException
{
  public:
    explicit TooManyThreadsException() : ArionException("Too many threads are already active.") {};
};

class WrongThreadIdException : public ArionException
{
  public:
    explicit WrongThreadIdException() : ArionException("Specified thread id does not exist.") {};
};

class UnknownSignalException : public ArionException
{
  public:
    explicit UnknownSignalException(pid_t pid, pid_t tid, int signo)
        : ArionException(std::string("Thread ") + int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         int_to_hex<pid_t>(pid) + std::string(" received unknown signal \"") + int_to_hex<int>(signo) +
                         std::string("\".")) {};
};

class NoSighandlerForSignalException : public ArionException
{
  public:
    explicit NoSighandlerForSignalException(pid_t pid, pid_t tid, int signo)
        : ArionException(std::string("Thread ") + int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         int_to_hex<pid_t>(pid) + std::string(" has no signal handler set for signal ") +
                         int_to_hex<int>(signo) + std::string(".")) {};
};

class UnhandledSyncSignalException : public ArionException
{
  public:
    explicit UnhandledSyncSignalException(pid_t pid, pid_t tid, std::string sig_desc)
        : ArionException(std::string("Thread ") + int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         int_to_hex<pid_t>(pid) + std::string(" crashed with unhandled signal \"") + sig_desc +
                         std::string("\".")) {};
};

class ThreadAlreadySigWaitingException : public ArionException
{
  public:
    explicit ThreadAlreadySigWaitingException(pid_t pid, pid_t tid)
        : ArionException(std::string("Thread ") + int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         int_to_hex<pid_t>(pid) + std::string(" is already waiting for a signal.")) {};
};

class WaitSameProcessException : public ArionException
{
  public:
    explicit WaitSameProcessException(pid_t pid)
        : ArionException(std::string("Process ") + int_to_hex<pid_t>(pid) + std::string(" can't wait for itself.")) {};
};

class NoProcessWithPidException : public ArionException
{
  public:
    explicit NoProcessWithPidException(pid_t pid)
        : ArionException(std::string("Process ") + int_to_hex<pid_t>(pid) +
                         std::string(" does not exist or has exited.")) {};
};

class NoChildWithPidException : public ArionException
{
  public:
    explicit NoChildWithPidException(pid_t parent_pid, pid_t child_pid)
        : ArionException(std::string("Parent process (") + int_to_hex<pid_t>(parent_pid) +
                         std::string(") has no child with PID : ") + int_to_hex<pid_t>(child_pid) +
                         std::string(".")) {};
};

class WrongHookParamsException : public ArionException
{
  public:
    explicit WrongHookParamsException() : ArionException("Wrong hook parameters.") {};
};

class TooManyHooksException : public ArionException
{
  public:
    explicit TooManyHooksException() : ArionException("Too many hooks are already active.") {};
};

class WrongHookIdException : public ArionException
{
  public:
    explicit WrongHookIdException() : ArionException("Specified hook id does not exist.") {};
};

class HeavierRegException : public ArionException
{
  public:
    explicit HeavierRegException(uint8_t req_sz, uint8_t prov_sz)
        : ArionException(std::string("Can't read/write a ") + int_to_hex<uint8_t>(req_sz) +
                         std::string(" bytes register with a ") + int_to_hex<uint8_t>(prov_sz) +
                         std::string(" bytes variable.")) {};
};

class NoRegWithValueException : public ArionException
{
  public:
    explicit NoRegWithValueException(arion::REG reg)
        : ArionException(std::string("Register with value \"") + int_to_hex<uint64_t>(reg) +
                         std::string("\" does not exist in current architecture.")) {};
};

class NoRegWithNameException : public ArionException
{
  public:
    explicit NoRegWithNameException(std::string reg_name)
        : ArionException(std::string("Register with name \"") + reg_name +
                         std::string("\" does not exist in current architecture.")) {};
};

class NoStructFieldWithNameException : public ArionException
{
  public:
    explicit NoStructFieldWithNameException(std::string field_name)
        : ArionException(std::string("This structure has no field called \"") + field_name + std::string("\".")) {};
};

class TooManyStructsException : public ArionException
{
  public:
    explicit TooManyStructsException() : ArionException("Too many structures are already active.") {};
};

class WrongStructIdException : public ArionException
{
  public:
    explicit WrongStructIdException() : ArionException("Specified struct id does not exist.") {};
};

class NoIdtEntryWithIntnoException : public ArionException
{
  public:
    explicit NoIdtEntryWithIntnoException(uint32_t intno)
        : ArionException(std::string("Interrupt number ") + int_to_hex<uint32_t>(intno) +
                         std::string(" is not referenced in IDT.")) {};
};

class NoSignalForIntrException : public ArionException
{
  public:
    explicit NoSignalForIntrException()
        : ArionException(
              std::string("CPU interrupt does not exist or has no corresponding kernel signal implemented.")) {};
};

class UnicornOpenException : public ArionException
{
  public:
    explicit UnicornOpenException(uc_err err)
        : ArionException(std::string("An error occurred while opening Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

class UnicornCtlException : public ArionException
{
  public:
    explicit UnicornCtlException(uc_err err)
        : ArionException(std::string("An error occurred while configuring Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

class UnicornMapException : public ArionException
{
  public:
    explicit UnicornMapException(uc_err err)
        : ArionException(std::string("An error occurred while mapping a memory segment with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornUnmapException : public ArionException
{
  public:
    explicit UnicornUnmapException(uc_err err)
        : ArionException(std::string("An error occurred while unmapping a memory segment with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornMemRegionsException : public ArionException
{
  public:
    explicit UnicornMemRegionsException(uc_err err)
        : ArionException(std::string("An error occurred while fetching memory regions with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornMemReadException : public ArionException
{
  public:
    explicit UnicornMemReadException(uc_err err)
        : ArionException(std::string("An error occurred while reading from memory with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornMemWriteException : public ArionException
{
  public:
    explicit UnicornMemWriteException(uc_err err)
        : ArionException(std::string("An error occurred while writing to memory with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornMemProtectException : public ArionException
{
  public:
    explicit UnicornMemProtectException(uc_err err)
        : ArionException(
              std::string("An error occurred while changing memory writes of segment with Unicorn engine : \"") +
              uc_strerror(err) + std::string("\".")) {};
};

class UnicornRegReadException : public ArionException
{
  public:
    explicit UnicornRegReadException(uc_err err)
        : ArionException(std::string("An error occurred while reading a register with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornRegWriteException : public ArionException
{
  public:
    explicit UnicornRegWriteException(uc_err err)
        : ArionException(std::string("An error occurred while writing a register with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornCtlFlushTbException : public ArionException
{
  public:
    explicit UnicornCtlFlushTbException(uc_err err)
        : ArionException(
              std::string("An error occurred while flushing Qemu translation blocks with Unicorn engine : \"") +
              uc_strerror(err) + std::string("\".")) {};
};

class UnicornHookAddException : public ArionException
{
  public:
    explicit UnicornHookAddException(uc_err err)
        : ArionException(std::string("An error occurred while injecting a hook with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornHookDelException : public ArionException
{
  public:
    explicit UnicornHookDelException(uc_err err)
        : ArionException(std::string("An error occurred while removing a hook with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

class UnicornRunException : public ArionException
{
  public:
    explicit UnicornRunException(uc_err err)
        : ArionException(std::string("An error occurred while running Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

class UnicornStopException : public ArionException
{
  public:
    explicit UnicornStopException(uc_err err)
        : ArionException(std::string("An error occurred while stopping Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

class UnicornAflException : public ArionException
{
  public:
    explicit UnicornAflException(std::string err)
        : ArionException(std::string("An error occurred while using UnicornAFL engine : \"") + err +
                         std::string("\".")) {};
};

class UnicornAflNoExitsException : public ArionException
{
  public:
    explicit UnicornAflNoExitsException()
        : ArionException(std::string("At least one exit must be specified when fuzzing with UnicornAFL engine.")) {};
};

class KeystoneOpenException : public ArionException
{
  public:
    explicit KeystoneOpenException(ks_err err)
        : ArionException(std::string("An error occurred while opening Keystone engine : \"") + ks_strerror(err) +
                         std::string("\".")) {};
};

class KeystoneAsmException : public ArionException
{
  public:
    explicit KeystoneAsmException(ks_err err)
        : ArionException(std::string("An error occurred while assembling with Keystone engine : \"") +
                         ks_strerror(err) + std::string("\".")) {};
};

class CapstoneOpenException : public ArionException
{
  public:
    explicit CapstoneOpenException(cs_err err)
        : ArionException(std::string("An error occurred while opening Capstone engine : \"") + cs_strerror(err) +
                         std::string("\".")) {};
};

class FileAlreadyHasFdException : public ArionException
{
  public:
    explicit FileAlreadyHasFdException(int fd)
        : ArionException(std::string("Cannot add entry. File descriptor ") + int_to_hex<int>(fd) +
                         std::string(" already references file.")) {};
};

class NoFileAtFdException : public ArionException
{
  public:
    explicit NoFileAtFdException(int fd)
        : ArionException(std::string("Cannot find file entry with file descriptor ") + int_to_hex<int>(fd) +
                         std::string(".")) {};
};

class SocketAlreadyHasFdException : public ArionException
{
  public:
    explicit SocketAlreadyHasFdException(int fd)
        : ArionException(std::string("Cannot add entry. File descriptor ") + int_to_hex<int>(fd) +
                         std::string(" already references socket.")) {};
};

class NoSocketAtFdException : public ArionException
{
  public:
    explicit NoSocketAtFdException(int fd)
        : ArionException(std::string("Cannot find socket entry with file descriptor ") + int_to_hex<int>(fd) +
                         std::string(".")) {};
};

class NoFreeGdtEntryException : public ArionException
{
  public:
    explicit NoFreeGdtEntryException()
        : ArionException(std::string("A free GDT entry was requested and could not be found.")) {};
};

class WrongTraceFileMagicException : public ArionException
{
  public:
    explicit WrongTraceFileMagicException(std::string file_path)
        : ArionException(std::string("File \"") + file_path +
                         std::string("\" does not start with required magic sequence for a trace file.")) {};
};

class NewerTraceFileVersionException : public ArionException
{
  public:
    explicit NewerTraceFileVersionException(std::string file_path)
        : ArionException(
              std::string("Trace file \"") + file_path +
              std::string("\" was written with a newer version of Arion. Consider updating Arion to use it.")) {};
};

class UnknownTraceModeException : public ArionException
{
  public:
    explicit UnknownTraceModeException() : ArionException(std::string("Specified trace mode does not exist.")) {};
};

class DifferentTraceModesException : public ArionException
{
  public:
    explicit DifferentTraceModesException()
        : ArionException(std::string("Unable to compare code traces that use different trace modes.")) {};
};

class WrongTraceModeException : public ArionException
{
  public:
    explicit WrongTraceModeException()
        : ArionException(std::string("Requested feature can't be used with specified trace mode.")) {};
};

class UnknownTraceRegException : public ArionException
{
  public:
    explicit UnknownTraceRegException(arion::REG reg)
        : ArionException(
              std::string("Register ") + int_to_hex<arion::REG>(reg) +
              std::string(" is not part of trace file. Only the largest registers are written to trace files.")) {};
};

class UnknownTraceModuleIdException : public ArionException
{
  public:
    explicit UnknownTraceModuleIdException(std::string file_path, uint16_t mod_id)
        : ArionException(std::string("Trace file \"") + file_path +
                         std::string("\" does not contain a module with ID : ") + int_to_hex<uint16_t>(mod_id) +
                         std::string(".")) {};
};

class UnknownTraceModuleNameException : public ArionException
{
  public:
    explicit UnknownTraceModuleNameException(std::string file_path, std::string name)
        : ArionException(std::string("Trace file \"") + file_path +
                         std::string("\" does not contain a module with name : \"") + name + std::string("\".")) {};
};

class UnknownTraceModuleHashException : public ArionException
{
  public:
    explicit UnknownTraceModuleHashException(std::string file_path, std::string hash)
        : ArionException(std::string("Trace file \"") + file_path +
                         std::string("\" does not contain a module with hash : \"") + hash + std::string("\".")) {};
};

class CantReachTraceAddrException : public ArionException
{
  public:
    explicit CantReachTraceAddrException(arion::ADDR addr)
        : ArionException(std::string("Can't reach address ") + int_to_hex<arion::ADDR>(addr) +
                         std::string(" while reading trace file.")) {};
};

class CantReachTraceOffException : public ArionException
{
  public:
    explicit CantReachTraceOffException(std::string name, uint32_t off)
        : ArionException(std::string("Can't reach ") + name + std::string(" + ") + int_to_hex<uint32_t>(off) +
                         std::string(" while reading trace file.")) {};
};

class TracerAlreadyEnabledException : public ArionException
{
  public:
    explicit TracerAlreadyEnabledException() : ArionException(std::string("Tracer is already enabled.")) {};
};

class TracerAlreadyDisabledException : public ArionException
{
  public:
    explicit TracerAlreadyDisabledException() : ArionException(std::string("Tracer is already disabled.")) {};
};

class MemoryRecorderAlreadyStartedException : public ArionException
{
  public:
    explicit MemoryRecorderAlreadyStartedException()
        : ArionException(std::string("MemoryRecorder is already started.")) {};
};

class MemoryRecorderAlreadyStoppedException : public ArionException
{
  public:
    explicit MemoryRecorderAlreadyStoppedException()
        : ArionException(std::string("MemoryRecorder is already stopped.")) {};
};

class WrongLogLevelException : public ArionException
{
  public:
    explicit WrongLogLevelException() : ArionException(std::string("Specified log level does not exist.")) {};
};

class TooManyLoggersException : public ArionException
{
  public:
    explicit TooManyLoggersException() : ArionException("Too many loggers are already active.") {};
};

#endif // ARION_GLOBAL_EXCEPTS_HPP
