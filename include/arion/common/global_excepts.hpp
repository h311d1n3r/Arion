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

#define ARION_EXCEPTION_MAX_FRAMES 64

namespace arion_exception
{

/// This class represents a custom exception defined in Arion environment.
class ArionException : public std::exception
{
  private:
    /// The reason why this exception was triggered.
    std::string message;

  public:
    /*
     * Builder for ArionException instances.
     * @param[in] msg The reason why this exception was triggered.
     */
    ArionException(std::string msg) : message(msg)
    {
        void *buffer[ARION_EXCEPTION_MAX_FRAMES];
        int frame_count = backtrace(buffer, ARION_EXCEPTION_MAX_FRAMES);
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
                   << (info.dli_sname ? arion::demangle_cxx_symbol(std::string(info.dli_sname)) : "??") << " + "
                   << arion::int_to_hex<uintptr_t>(offset) << " (" << buffer[i] << ") at "
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

    /**
     * Overridden from std::exception. Returns the reason why this exception was triggered.
     * @return The reason why this exception was triggered.
     */
    const char *what() const noexcept override
    {
        return message.c_str();
    }
};

/// Thrown when passing an invalid argument to a method.
class InvalidArgumentException : public ArionException
{
  public:
    /**
     * Builder for InvalidArgumentException instances.
     * @param[in] err Name of the parameter with an invalid value.
     */
    explicit InvalidArgumentException(std::string err)
        : ArionException(std::string("Invalid argument : \"") + err + std::string("\".")) {};
};

/// Thrown when attempting to use an expired weak_ptr.
class ExpiredWeakPtrException : public ArionException
{
  public:
    /**
     * Builder for ExpiredWeakPtrException instances.
     * @param[in] weak_ptr_name Name of the expired weak_ptr.
     */
    explicit ExpiredWeakPtrException(std::string weak_ptr_name)
        : ArionException(std::string("Attempting to use an expired weak_ptr : ") + weak_ptr_name + std::string(".")) {};
};

/// Thrown when a file is not found.
class FileNotFoundException : public ArionException
{
  public:
    /**
     * Builder for FileNotFoundException instances.
     * @param[in] file_path Path to the file that could not be found.
     */
    explicit FileNotFoundException(std::string file_path)
        : ArionException(std::string("File could not be found \"") + file_path + std::string("\".")) {};
};

/// Thrown when an error occurs while opening a file.
class FileOpenException : public ArionException
{
  public:
    /**
     * Builder for FileOpenException instances.
     * @param[in] file_path Path of the file that could not be opened.
     */
    explicit FileOpenException(std::string file_path)
        : ArionException(std::string("An error occurred while opening file \"") + file_path + std::string("\".")) {};
};

/// Thrown when a file is smaller than expected.
class FileTooSmallException : public ArionException
{
  public:
    /**
     * Builder for FileTooSmallException instances.
     * @param[in] file_path Path of the file with invalid size.
     * @param[in] file_sz Current size of the file.
     * @param[in] min_file_sz Minimum size the file should be.
     */
    explicit FileTooSmallException(std::string file_path, size_t file_sz, size_t min_file_sz)
        : ArionException(std::string("File size of \"") + file_path + std::string("\" is ") +
                         arion::int_to_hex<size_t>(file_sz) + std::string(" when it should be at least ") +
                         arion::int_to_hex<size_t>(min_file_sz) + std::string(".")) {};
};

/// Thrown when a file is not found in a file system.
class FileNotInFsException : public ArionException
{
  public:
    /**
     * Builder for FileNotInFsException instances.
     * @param[in] fs Name of the file system where the file is not found.
     * @param[in] file_path Path of the file that is not found.
     */
    explicit FileNotInFsException(std::string fs, std::string file_path)
        : ArionException(std::string("File \"") + file_path + std::string("\" is not in file system \"") + fs +
                         std::string("\".")) {};
};

/// Thrown when a key is not found in the configuration.
class ConfigKeyNotFoundException : public ArionException
{
  public:
    /**
     * Builder for ConfigKeyNotFoundException instances.
     * @param[in] key Name of the configuration key that was not found.
     */
    explicit ConfigKeyNotFoundException(std::string key)
        : ArionException(std::string("Configuration doesn't have a \"") + key + std::string("\" key.")) {};
};

/// Thrown when trying to access a configuration key with the wrong type.
class ConfigWrongTypeAccessException : public ArionException
{
  public:
    /**
     * Builder for ConfigWrongTypeAccessException instances.
     * @param[in] key Name of the configuration key with an invalid type.
     */
    explicit ConfigWrongTypeAccessException(std::string key)
        : ArionException(std::string("Configuration key \"") + key +
                         std::string("\" was accessed with a wrong type.")) {};
};

/// Thrown when a file does not start with the required magic sequence for a context file.
class WrongContextFileMagicException : public ArionException
{
  public:
    /**
     * Builder for WrongContextFileMagicException instances.
     * @param[in] file_path Path to the file with an invalid magic sequence.
     */
    explicit WrongContextFileMagicException(std::string file_path)
        : ArionException(std::string("File \"") + file_path +
                         std::string("\" does not start with required magic sequence for a context file.")) {};
};

/// Thrown when a context file was written with a newer version of Arion.
class NewerContextFileVersionException : public ArionException
{
  public:
    /**
     * Builder for NewerContextFileVersionException instances.
     * @param[in] file_path Path to the context file.
     */
    explicit NewerContextFileVersionException(std::string file_path)
        : ArionException(
              std::string("Context file \"") + file_path +
              std::string("\" was written with a newer version of Arion. Consider updating Arion to use it.")) {};
};

/// Thrown when a file path is too long.
class PathTooLongException : public ArionException
{
  public:
    /**
     * Builder for PathTooLongException instances.
     * @param[in] file_path The file path that is too long.
     */
    explicit PathTooLongException(std::string file_path)
        : ArionException(std::string("File path \"") + file_path + std::string("\" is too long.")) {};
};

/// Thrown when the linkage type of an executable cannot be identified.
class UnknownLinkageTypeException : public ArionException
{
  public:
    /**
     * Builder for UnknownLinkageTypeException instances.
     * @param[in] exe_path Path of the executable for which the linkage type could not be identified.
     */
    explicit UnknownLinkageTypeException(std::string exe_path)
        : ArionException(std::string("Couldn't identify linkage type of \"") + exe_path + std::string("\".")) {};
};

/// Thrown when passing an invalid system call number to a method.
class InvalidSyscallNoException : public ArionException
{
  public:
    /**
     * Builder for InvalidSyscallNoException instances.
     * @param[in] syscall_no The invalid system call number.
     */
    explicit InvalidSyscallNoException(uint64_t syscall_no)
        : ArionException(std::string("Invalid syscall number : ") + arion::int_to_hex(syscall_no) + std::string(".")) {
          };
};

/// Thrown when passing an invalid system call name argument to a method.
class InvalidSyscallNameException : public ArionException
{
  public:
    /**
     * Builder for InvalidSyscallNameException instances.
     * @param[in] name Name of the system call with an invalid value.
     */
    explicit InvalidSyscallNameException(std::string &name)
        : ArionException(std::string("Invalid syscall name : \"") + name + std::string("\".")) {};
};

/// Thrown when the linkage type is not suitable for the operation.
class BadLinkageTypeException : public ArionException
{
  public:
    /**
     * Builder for BadLinkageTypeException instances.
     * @param[in] exe_path Name of the executable with an invalid linkage type.
     */
    explicit BadLinkageTypeException(std::string exe_path)
        : ArionException(std::string("Wrong linkage type for \"") + exe_path + std::string("\".")) {};
};

/// Thrown when trying to analyze a coredump note that does not refer to a thread.
class NoCoredumpCurrentThreadException : public ArionException
{
  public:
    /**
     * Builder for NoCoredumpCurrentThreadException instances.
     */
    explicit NoCoredumpCurrentThreadException()
        : ArionException(std::string("Can't analyze coredump note because it does not refer to a thread.")) {};
};

/// Thrown when there is no memory segment mapped at the specified address.
class NoSegmentAtAddrException : public ArionException
{
  public:
    /**
     * Builder for NoSegmentAtAddrException instances.
     * @param[in] addr The address where no memory segment is mapped.
     */
    explicit NoSegmentAtAddrException(arion::ADDR addr)
        : ArionException(std::string("No memory segment is mapped at address ") + arion::int_to_hex(addr) +
                         std::string(".")) {};
};

/// Thrown when there is no memory segment with the specified info.
class NoSegmentWithInfoException : public ArionException
{
  public:
    /**
     * Builder for NoSegmentWithInfoException instances.
     * @param[in] info The info of the memory segment that is not found.
     */
    explicit NoSegmentWithInfoException(std::string info)
        : ArionException(std::string("No memory segment has info ") + info + std::string(".")) {};
};

/// Thrown when attempting to access a memory segment that has not been mapped.
class SegmentNotMappedException : public ArionException
{
  public:
    /**
     * Builder for SegmentNotMappedException instances.
     * @param[in] start_addr The start address of the segment.
     * @param[in] end_addr The end address of the segment.
     */
    explicit SegmentNotMappedException(arion::ADDR start_addr, arion::ADDR end_addr)
        : ArionException(std::string("Segment [") + arion::int_to_hex(start_addr) + std::string(":") +
                         arion::int_to_hex(end_addr) + std::string("] is not mapped in memory.")) {};
};

/// Thrown when attempting to map memory that has already been mapped.
class MemAlreadyMappedException : public ArionException
{
  public:
    /**
     * Builder for MemAlreadyMappedException instances.
     * @param[in] start_addr The starting address of the memory that was attempted to be mapped.
     * @param[in] sz The size of the memory that was attempted to be mapped.
     */
    explicit MemAlreadyMappedException(arion::ADDR start_addr, size_t sz)
        : ArionException(std::string("An error occurred while attempting to map memory at address ") +
                         arion::int_to_hex<arion::ADDR>(start_addr) + std::string(" with size ") +
                         arion::int_to_hex<ssize_t>(sz) + std::string(".")) {};
};

/// Thrown when trying to unmap a memory segment that is not within the bounds of the segment.
class CantUnmapOutsideSegmentException : public ArionException
{
  public:
    /**
     * Builder for CantUnmapOutsideSegmentException instances.
     * @param[in] seg_start Start address of the segment.
     * @param[in] seg_end End address of the segment.
     * @param[in] unmap_start Start address of the unmapping operation.
     * @param[in] unmap_end End address of the unmapping operation.
     */
    explicit CantUnmapOutsideSegmentException(arion::ADDR seg_start, arion::ADDR seg_end, arion::ADDR unmap_start,
                                              arion::ADDR unmap_end)
        : ArionException(std::string("Segment [") + arion::int_to_hex(seg_start) + std::string(":") +
                         arion::int_to_hex(seg_end) + std::string("] is not between unmapping bounds [") +
                         arion::int_to_hex(unmap_start) + std::string(":") + arion::int_to_hex(unmap_end) +
                         std::string("].")) {};
};

/// Thrown when the CPU architecture is not supported.
class UnsupportedCpuArchException : public ArionException
{
  public:
    /**
     * Builder for UnsupportedCpuArchException instances.
     */
    explicit UnsupportedCpuArchException()
        : ArionException("This CPU architecture in not supported for the moment.") {};
};

/// Thrown when the host CPU architecture is not supported.
class UnsupportedHostCpuArchException : public ArionException
{
  public:
    /**
     * Builder for UnsupportedHostCpuArchException instances.
     */
    explicit UnsupportedHostCpuArchException()
        : ArionException("Your host CPU architecture in not supported for the moment.") {};
};

/// Thrown when the system is already running the maximum number of threads.
class TooManyThreadsException : public ArionException
{
  public:
    /**
     * Builder for TooManyThreadsException instances.
     */
    explicit TooManyThreadsException() : ArionException("Too many threads are already active.") {};
};

/// Thrown when attempting to access a thread with an invalid thread id.
class WrongThreadIdException : public ArionException
{
  public:
    /**
     * Builder for WrongThreadIdException instances.
     */
    explicit WrongThreadIdException() : ArionException("Specified thread id does not exist.") {};
};

/// Thrown when a thread receives an unknown signal.
class UnknownSignalException : public ArionException
{
  public:
    /**
     * Builder for UnknownSignalException instances.
     * @param[in] pid Process ID of the thread.
     * @param[in] tid Thread ID of the thread.
     * @param[in] signo Signal number that was received.
     */
    explicit UnknownSignalException(pid_t pid, pid_t tid, int signo)
        : ArionException(std::string("Thread ") + arion::int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         arion::int_to_hex<pid_t>(pid) + std::string(" received unknown signal \"") +
                         arion::int_to_hex<int>(signo) + std::string("\".")) {};
};

/// Thrown when a thread has no signal handler set for a given signal.
class NoSighandlerForSignalException : public ArionException
{
  public:
    /**
     * Builder for NoSighandlerForSignalException instances.
     * @param[in] pid The process id.
     * @param[in] tid The thread id.
     * @param[in] signo The signal number.
     */
    explicit NoSighandlerForSignalException(pid_t pid, pid_t tid, int signo)
        : ArionException(std::string("Thread ") + arion::int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         arion::int_to_hex<pid_t>(pid) + std::string(" has no signal handler set for signal ") +
                         arion::int_to_hex<int>(signo) + std::string(".")) {};
};

/// Thrown when a thread crashes with an unhandled signal.
class UnhandledSyncSignalException : public ArionException
{
  public:
    /**
     * Builder for UnhandledSyncSignalException instances.
     * @param[in] pid Process ID of the crashed thread.
     * @param[in] tid Thread ID of the crashed thread.
     * @param[in] sig_desc Description of the unhandled signal.
     */
    explicit UnhandledSyncSignalException(pid_t pid, pid_t tid, std::string sig_desc)
        : ArionException(std::string("Thread ") + arion::int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         arion::int_to_hex<pid_t>(pid) + std::string(" crashed with unhandled signal \"") + sig_desc +
                         std::string("\".")) {};
};

/// Thrown when a thread is already waiting for a signal.
class ThreadAlreadySigWaitingException : public ArionException
{
  public:
    /**
     * Builder for ThreadAlreadySigWaitingException instances.
     * @param[in] pid Process ID of the thread.
     * @param[in] tid Thread ID of the thread.
     */
    explicit ThreadAlreadySigWaitingException(pid_t pid, pid_t tid)
        : ArionException(std::string("Thread ") + arion::int_to_hex<pid_t>(tid) + std::string(" from process ") +
                         arion::int_to_hex<pid_t>(pid) + std::string(" is already waiting for a signal.")) {};
};

/// Thrown when trying to wait for a process that is the same as the current process.
class WaitSameProcessException : public ArionException
{
  public:
    /**
     * Builder for WaitSameProcessException instances.
     * @param[in] pid The PID of the process that is trying to wait for itself.
     */
    explicit WaitSameProcessException(pid_t pid)
        : ArionException(std::string("Process ") + arion::int_to_hex<pid_t>(pid) +
                         std::string(" can't wait for itself.")) {};
};

/// Thrown when trying to access a process with a non-existing or already exited PID.
class NoProcessWithPidException : public ArionException
{
  public:
    /**
     * Builder for NoProcessWithPidException instances.
     * @param[in] pid The PID of the process that does not exist or has exited.
     */
    explicit NoProcessWithPidException(pid_t pid)
        : ArionException(std::string("Process ") + arion::int_to_hex<pid_t>(pid) +
                         std::string(" does not exist or has exited.")) {};
};

/// Thrown when a parent process does not have a child process with a specific PID.
class NoChildWithPidException : public ArionException
{
  public:
    /**
     * Builder for NoChildWithPidException instances.
     * @param[in] parent_pid The PID of the parent process.
     * @param[in] child_pid The PID of the child process.
     */
    explicit NoChildWithPidException(pid_t parent_pid, pid_t child_pid)
        : ArionException(std::string("Parent process (") + arion::int_to_hex<pid_t>(parent_pid) +
                         std::string(") has no child with PID : ") + arion::int_to_hex<pid_t>(child_pid) +
                         std::string(".")) {};
};

/// Thrown when the parameters passed to a hook are invalid.
class WrongHookParamsException : public ArionException
{
  public:
    /**
     * Builder for WrongHookParamsException instances.
     */
    explicit WrongHookParamsException() : ArionException("Wrong hook parameters.") {};
};

/// Thrown when there are too many hooks already active.
class TooManyHooksException : public ArionException
{
  public:
    /**
     * Builder for TooManyHooksException instances.
     */
    explicit TooManyHooksException() : ArionException("Too many hooks are already active.") {};
};

/// Thrown when attempting to use a non-existent hook id.
class WrongHookIdException : public ArionException
{
  public:
    /**
     * Builder for WrongHookIdException instances.
     */
    explicit WrongHookIdException() : ArionException("Specified hook id does not exist.") {};
};

/// Thrown when attempting to read/write a register with a size that doesn't match the register's size.
class HeavierRegException : public ArionException
{
  public:
    /**
     * Builder for HeavierRegException instances.
     * @param[in] req_sz Size of the register required.
     * @param[in] prov_sz Size of the provided variable.
     */
    explicit HeavierRegException(uint8_t req_sz, uint8_t prov_sz)
        : ArionException(std::string("Can't read/write a ") + arion::int_to_hex<uint8_t>(req_sz) +
                         std::string(" bytes register with a ") + arion::int_to_hex<uint8_t>(prov_sz) +
                         std::string(" bytes variable.")) {};
};

/// Thrown when trying to access a register that does not exist in the current architecture.
class NoRegWithValueException : public ArionException
{
  public:
    /**
     * Builder for NoRegWithValueException instances.
     * @param[in] reg Value of the register that does not exist.
     */
    explicit NoRegWithValueException(arion::REG reg)
        : ArionException(std::string("Register with value ") + arion::int_to_hex<uint64_t>(reg) +
                         std::string(" does not exist in current architecture.")) {};
};

/// Thrown when trying to access a register that does not exist in the current architecture.
class NoRegWithNameException : public ArionException
{
  public:
    /**
     * Builder for NoRegWithNameException instances.
     * @param[in] reg_name Name of the register that does not exist.
     */
    explicit NoRegWithNameException(std::string reg_name)
        : ArionException(std::string("Register with name \"") + reg_name +
                         std::string("\" does not exist in current architecture.")) {};
};

/// Thrown when attempting to access a field in a structure that does not exist.
class NoStructFieldWithNameException : public ArionException
{
  public:
    /**
     * Builder for NoStructFieldWithNameException instances.
     * @param[in] field_name Name of the field that does not exist.
     */
    explicit NoStructFieldWithNameException(std::string field_name)
        : ArionException(std::string("This structure has no field called \"") + field_name + std::string("\".")) {};
};

/// Thrown when there are too many active structures.
class TooManyStructsException : public ArionException
{
  public:
    /**
     * Builder for TooManyStructsException instances.
     */
    explicit TooManyStructsException() : ArionException("Too many structures are already active.") {};
};

/// Thrown when an invalid struct id is specified.
class WrongStructIdException : public ArionException
{
  public:
    /**
     * Builder for WrongStructIdException instances.
     */
    explicit WrongStructIdException() : ArionException("Specified struct id does not exist.") {};
};

/// Thrown when attempting to access an IDT entry with an interrupt number that is not referenced.
class NoIdtEntryWithIntnoException : public ArionException
{
  public:
    /**
     * Builder for NoIdtEntryWithIntnoException instances.
     * @param[in] intno The interrupt number that is not referenced.
     */
    explicit NoIdtEntryWithIntnoException(uint32_t intno)
        : ArionException(std::string("Interrupt number ") + arion::int_to_hex<uint32_t>(intno) +
                         std::string(" is not referenced in IDT.")) {};
};

/// Thrown when a CPU interrupt does not exist or has no corresponding kernel signal implemented.
class NoSignalForIntrException : public ArionException
{
  public:
    /**
     * Builder for NoSignalForIntrException instances.
     */
    explicit NoSignalForIntrException()
        : ArionException(
              std::string("CPU interrupt does not exist or has no corresponding kernel signal implemented.")) {};
};

/// Thrown when an error occurs while opening Unicorn engine.
class UnicornOpenException : public ArionException
{
  public:
    /**
     * Builder for UnicornOpenException instances.
     * @param[in] err The error code returned by Unicorn engine.
     */
    explicit UnicornOpenException(uc_err err)
        : ArionException(std::string("An error occurred while opening Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

/// Thrown when an error occurs while configuring the Unicorn engine.
class UnicornCtlException : public ArionException
{
  public:
    /**
     * Builder for UnicornCtlException instances.
     * @param[in] err Error code returned by the Unicorn engine.
     */
    explicit UnicornCtlException(uc_err err)
        : ArionException(std::string("An error occurred while configuring Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

/// Thrown when an error occurs while mapping a memory segment with the Unicorn engine.
class UnicornMapException : public ArionException
{
  public:
    /**
     * Builder for UnicornMapException instances.
     * @param[in] err Error code returned by the Unicorn engine.
     */
    explicit UnicornMapException(uc_err err)
        : ArionException(std::string("An error occurred while mapping a memory segment with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while unmapping a memory segment with Unicorn engine.
class UnicornUnmapException : public ArionException
{
  public:
    /**
     * Builder for UnicornUnmapException instances.
     * @param[in] err Error code returned by Unicorn engine.
     */
    explicit UnicornUnmapException(uc_err err)
        : ArionException(std::string("An error occurred while unmapping a memory segment with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while fetching memory regions with Unicorn engine.
class UnicornMemRegionsException : public ArionException
{
  public:
    /**
     * Builder for UnicornMemRegionsException instances.
     * @param[in] err Error code returned by Unicorn engine.
     */
    explicit UnicornMemRegionsException(uc_err err)
        : ArionException(std::string("An error occurred while fetching memory regions with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while reading from memory with Unicorn engine.
class UnicornMemReadException : public ArionException
{
  public:
    /**
     * Builder for UnicornMemReadException instances.
     * @param[in] err Error code from Unicorn engine.
     */
    explicit UnicornMemReadException(uc_err err)
        : ArionException(std::string("An error occurred while reading from memory with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while writing to memory with the Unicorn engine.
class UnicornMemWriteException : public ArionException
{
  public:
    /**
     * Builder for UnicornMemWriteException instances.
     * @param[in] err The error code returned by the Unicorn engine.
     */
    explicit UnicornMemWriteException(uc_err err)
        : ArionException(std::string("An error occurred while writing to memory with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while changing memory writes of a segment with Unicorn engine.
class UnicornMemProtectException : public ArionException
{
  public:
    /**
     * Builder for UnicornMemProtectException instances.
     * @param[in] err Error code returned by Unicorn engine.
     */
    explicit UnicornMemProtectException(uc_err err)
        : ArionException(
              std::string("An error occurred while changing memory writes of segment with Unicorn engine : \"") +
              uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while reading a register with Unicorn engine.
class UnicornRegReadException : public ArionException
{
  public:
    /**
     * Builder for UnicornRegReadException instances.
     * @param[in] err Error code returned by Unicorn engine.
     */
    explicit UnicornRegReadException(uc_err err)
        : ArionException(std::string("An error occurred while reading a register with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while writing a register with Unicorn engine.
class UnicornRegWriteException : public ArionException
{
  public:
    /**
     * Builder for UnicornRegWriteException instances.
     * @param[in] err The error code returned by Unicorn engine.
     */
    explicit UnicornRegWriteException(uc_err err)
        : ArionException(std::string("An error occurred while writing a register with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while flushing Qemu translation blocks with Unicorn engine.
class UnicornCtlFlushTbException : public ArionException
{
  public:
    /**
     * Builder for UnicornCtlFlushTbException instances.
     * @param[in] err Error code returned by the Unicorn engine.
     */
    explicit UnicornCtlFlushTbException(uc_err err)
        : ArionException(
              std::string("An error occurred while flushing Qemu translation blocks with Unicorn engine : \"") +
              uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while injecting a hook with Unicorn engine.
class UnicornHookAddException : public ArionException
{
  public:
    /**
     * Builder for UnicornHookAddException instances.
     * @param[in] err Error code returned by Unicorn engine.
     */
    explicit UnicornHookAddException(uc_err err)
        : ArionException(std::string("An error occurred while injecting a hook with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while removing a hook with Unicorn engine.
class UnicornHookDelException : public ArionException
{
  public:
    /**
     * Builder for UnicornHookDelException instances.
     * @param[in] err Error code returned by Unicorn engine.
     */
    explicit UnicornHookDelException(uc_err err)
        : ArionException(std::string("An error occurred while removing a hook with Unicorn engine : \"") +
                         uc_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while running Unicorn engine.
class UnicornRunException : public ArionException
{
  public:
    /**
     * Builder for UnicornRunException instances.
     * @param[in] err The error code returned by Unicorn engine.
     */
    explicit UnicornRunException(uc_err err)
        : ArionException(std::string("An error occurred while running Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

/// Thrown when an error occurs while stopping Unicorn engine.
class UnicornStopException : public ArionException
{
  public:
    /**
     * Builder for UnicornStopException instances.
     * @param[in] err The error code returned by Unicorn engine.
     */
    explicit UnicornStopException(uc_err err)
        : ArionException(std::string("An error occurred while stopping Unicorn engine : \"") + uc_strerror(err) +
                         std::string("\".")) {};
};

/// Thrown when an error occurs while using the UnicornAFL engine.
class UnicornAflException : public ArionException
{
  public:
    /**
     * Builder for UnicornAflException instances.
     * @param[in] err Description of the error that occurred.
     */
    explicit UnicornAflException(std::string err)
        : ArionException(std::string("An error occurred while using UnicornAFL engine : \"") + err +
                         std::string("\".")) {};
};

/// Thrown when no exits are specified while fuzzing with UnicornAFL engine.
class UnicornAflNoExitsException : public ArionException
{
  public:
    /**
     * Builder for UnicornAflNoExitsException instances.
     */
    explicit UnicornAflNoExitsException()
        : ArionException(std::string("At least one exit must be specified when fuzzing with UnicornAFL engine.")) {};
};

/// Thrown when an error occurs while opening the Keystone engine.
class KeystoneOpenException : public ArionException
{
  public:
    /**
     * Builder for KeystoneOpenException instances.
     * @param[in] err The error code returned by Keystone engine.
     */
    explicit KeystoneOpenException(ks_err err)
        : ArionException(std::string("An error occurred while opening Keystone engine : \"") + ks_strerror(err) +
                         std::string("\".")) {};
};

/// Thrown when an error occurs while using the Keystone engine for assembly.
class KeystoneAsmException : public ArionException
{
  public:
    /**
     * Builder for KeystoneAsmException instances.
     * @param[in] err Error code returned by the Keystone engine.
     */
    explicit KeystoneAsmException(ks_err err)
        : ArionException(std::string("An error occurred while assembling with Keystone engine : \"") +
                         ks_strerror(err) + std::string("\".")) {};
};

/// Thrown when an error occurs while opening the Capstone engine.
class CapstoneOpenException : public ArionException
{
  public:
    /**
     * Builder for CapstoneOpenException instances.
     * @param[in] err The error code returned by the Capstone engine.
     */
    explicit CapstoneOpenException(cs_err err)
        : ArionException(std::string("An error occurred while opening Capstone engine : \"") + cs_strerror(err) +
                         std::string("\".")) {};
};

/// Thrown when attempting to add an entry to a file descriptor table that already contains an entry for the same file.
class FileAlreadyHasFdException : public ArionException
{
  public:
    /**
     * Builder for FileAlreadyHasFdException instances.
     * @param[in] fd File descriptor that is already in use.
     */
    explicit FileAlreadyHasFdException(int fd)
        : ArionException(std::string("Cannot add entry. File descriptor ") + arion::int_to_hex<int>(fd) +
                         std::string(" already references file.")) {};
};

/// Thrown when a file descriptor does not correspond to a file.
class NoFileAtFdException : public ArionException
{
  public:
    /**
     * Builder for NoFileAtFdException instances.
     * @param[in] fd File descriptor that does not correspond to a file.
     */
    explicit NoFileAtFdException(int fd)
        : ArionException(std::string("Cannot find file entry with file descriptor ") + arion::int_to_hex<int>(fd) +
                         std::string(".")) {};
};

/// Thrown when attempting to add an entry to a socket that already has a file descriptor.
class SocketAlreadyHasFdException : public ArionException
{
  public:
    /**
     * Builder for SocketAlreadyHasFdException instances.
     * @param[in] fd File descriptor that is already associated with a socket.
     */
    explicit SocketAlreadyHasFdException(int fd)
        : ArionException(std::string("Cannot add entry. File descriptor ") + arion::int_to_hex<int>(fd) +
                         std::string(" already references socket.")) {};
};

/// Thrown when a socket entry cannot be found with a given file descriptor.
class NoSocketAtFdException : public ArionException
{
  public:
    /**
     * Builder for NoSocketAtFdException instances.
     * @param[in] fd File descriptor of the socket entry.
     */
    explicit NoSocketAtFdException(int fd)
        : ArionException(std::string("Cannot find socket entry with file descriptor ") + arion::int_to_hex<int>(fd) +
                         std::string(".")) {};
};

/// Thrown when there is no free GDT entry available.
class NoFreeGdtEntryException : public ArionException
{
  public:
    /**
     * Builder for NoFreeGdtEntryException instances.
     */
    explicit NoFreeGdtEntryException()
        : ArionException(std::string("A free GDT entry was requested and could not be found.")) {};
};

/// Thrown when a trace file does not start with the required magic sequence.
class WrongTraceFileMagicException : public ArionException
{
  public:
    /**
     * Builder for WrongTraceFileMagicException instances.
     * @param[in] file_path Path to the file with an invalid magic sequence.
     */
    explicit WrongTraceFileMagicException(std::string file_path)
        : ArionException(std::string("File \"") + file_path +
                         std::string("\" does not start with required magic sequence for a trace file.")) {};
};

/// Thrown when a trace file was written with a newer version of Arion.
class NewerTraceFileVersionException : public ArionException
{
  public:
    /**
     * Builder for NewerTraceFileVersionException instances.
     * @param[in] file_path Path to the trace file.
     */
    explicit NewerTraceFileVersionException(std::string file_path)
        : ArionException(
              std::string("Trace file \"") + file_path +
              std::string("\" was written with a newer version of Arion. Consider updating Arion to use it.")) {};
};

/// Thrown when the specified trace mode does not exist.
class UnknownTraceModeException : public ArionException
{
  public:
    /**
     * Builder for UnknownTraceModeException instances.
     */
    explicit UnknownTraceModeException() : ArionException("Specified trace mode does not exist.") {};
};

/// Thrown when attempting to compare code traces that use different trace modes.
class DifferentTraceModesException : public ArionException
{
  public:
    /**
     * Builder for DifferentTraceModesException instances.
     */
    explicit DifferentTraceModesException()
        : ArionException(std::string("Unable to compare code traces that use different trace modes.")) {};
};

/// Thrown when the requested feature can't be used with the specified trace mode.
class WrongTraceModeException : public ArionException
{
  public:
    /**
     * Builder for WrongTraceModeException instances.
     */
    explicit WrongTraceModeException()
        : ArionException(std::string("Requested feature can't be used with specified trace mode.")) {};
};

/// Thrown when attempting to access a register that is not part of the trace file.
class UnknownTraceRegException : public ArionException
{
  public:
    /**
     * Builder for UnknownTraceRegException instances.
     * @param[in] reg The register that is not part of the trace file.
     */
    explicit UnknownTraceRegException(arion::REG reg)
        : ArionException(
              std::string("Register ") + arion::int_to_hex<arion::REG>(reg) +
              std::string(" is not part of trace file. Only the largest registers are written to trace files.")) {};
};

/// Thrown when attempting to access a module with an unknown ID in a trace file.
class UnknownTraceModuleIdException : public ArionException
{
  public:
    /**
     * Builder for UnknownTraceModuleIdException instances.
     * @param[in] file_path Path to the trace file.
     * @param[in] mod_id ID of the module that was not found.
     */
    explicit UnknownTraceModuleIdException(std::string file_path, uint16_t mod_id)
        : ArionException(std::string("Trace file \"") + file_path +
                         std::string("\" does not contain a module with ID : ") + arion::int_to_hex<uint16_t>(mod_id) +
                         std::string(".")) {};
};

/// Thrown when attempting to access a trace module with an unknown name.
class UnknownTraceModuleNameException : public ArionException
{
  public:
    /**
     * Builder for UnknownTraceModuleNameException instances.
     * @param[in] file_path The path of the trace file.
     * @param[in] name The name of the module that was not found.
     */
    explicit UnknownTraceModuleNameException(std::string file_path, std::string name)
        : ArionException(std::string("Trace file \"") + file_path +
                         std::string("\" does not contain a module with name : \"") + name + std::string("\".")) {};
};

/// Thrown when attempting to access a trace module with an unknown hash.
class UnknownTraceModuleHashException : public ArionException
{
  public:
    /**
     * Builder for UnknownTraceModuleHashException instances.
     * @param[in] file_path Path to the trace file.
     * @param[in] hash Hash of the module that was not found.
     */
    explicit UnknownTraceModuleHashException(std::string file_path, std::string hash)
        : ArionException(std::string("Trace file \"") + file_path +
                         std::string("\" does not contain a module with hash : \"") + hash + std::string("\".")) {};
};

/// Thrown when a trace address can't be reached.
class CantReachTraceAddrException : public ArionException
{
  public:
    /**
     * Builder for CantReachTraceAddrException instances.
     * @param[in] addr Address that can't be reached.
     */
    explicit CantReachTraceAddrException(arion::ADDR addr)
        : ArionException(std::string("Can't reach address ") + arion::int_to_hex<arion::ADDR>(addr) +
                         std::string(" while reading trace file.")) {};
};

/// Thrown when a trace file can't be reached at the specified offset.
class CantReachTraceOffException : public ArionException
{
  public:
    /**
     * Builder for CantReachTraceOffException instances.
     * @param[in] name Name of the trace file.
     * @param[in] off Offset where the trace file can't be reached.
     */
    explicit CantReachTraceOffException(std::string name, uint32_t off)
        : ArionException(std::string("Can't reach ") + name + std::string(" + ") + arion::int_to_hex<uint32_t>(off) +
                         std::string(" while reading trace file.")) {};
};

/// Thrown when attempting to enable a tracer that is already enabled.
class TracerAlreadyEnabledException : public ArionException
{
  public:
    /**
     * Builder for TracerAlreadyEnabledException instances.
     */
    explicit TracerAlreadyEnabledException() : ArionException(std::string("Tracer is already enabled.")) {};
    ;
};

/// Thrown when trying to disable a tracer that is already disabled.
class TracerAlreadyDisabledException : public ArionException
{
  public:
    /**
     * Builder for TracerAlreadyDisabledException instances.
     */
    explicit TracerAlreadyDisabledException() : ArionException(std::string("Tracer is already disabled.")) {};
};

/// Thrown when attempting to start a MemoryRecorder that is already started.
class MemoryRecorderAlreadyStartedException : public ArionException
{
  public:
    /**
     * Builder for MemoryRecorderAlreadyStartedException instances.
     */
    explicit MemoryRecorderAlreadyStartedException()
        : ArionException(std::string("MemoryRecorder is already started.")) {};
};

/// Thrown when trying to stop a MemoryRecorder that is already stopped.
class MemoryRecorderAlreadyStoppedException : public ArionException
{
  public:
    /**
     * Builder for MemoryRecorderAlreadyStoppedException instances.
     */
    explicit MemoryRecorderAlreadyStoppedException()
        : ArionException(std::string("MemoryRecorder is already stopped.")) {};
};

/// Thrown when the specified log level does not exist.
class WrongLogLevelException : public ArionException
{
  public:
    /**
     * Builder for WrongLogLevelException instances.
     */
    explicit WrongLogLevelException() : ArionException("Specified log level does not exist.") {};
};

/// Thrown when the maximum number of loggers has been reached.
class TooManyLoggersException : public ArionException
{
  public:
    /**
     * Builder for TooManyLoggersException instances.
     */
    explicit TooManyLoggersException() : ArionException("Too many loggers are already active.") {};
};

}; // namespace arion_exception

#endif // ARION_GLOBAL_EXCEPTS_HPP
