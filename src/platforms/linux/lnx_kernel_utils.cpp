#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <sys/mman.h>
#include <sys/socket.h>

using namespace arion;
using namespace arion_poly_struct;

std::map<uint64_t, std::string> errno_names = {
    {0, "SUCCESS"},
    {1, "EPERM"},
    {2, "ENOENT"},
    {3, "ESRCH"},
    {4, "EINTR"},
    {5, "EIO"},
    {6, "ENXIO"},
    {7, "E2BIG"},
    {8, "ENOEXEC"},
    {9, "EBADF"},
    {10, "ECHILD"},
    {11, "EAGAIN"},
    {12, "ENOMEM"},
    {13, "EACCES"},
    {14, "EFAULT"},
    {15, "ENOTBLK"},
    {16, "EBUSY"},
    {17, "EEXIST"},
    {18, "EXDEV"},
    {19, "ENODEV"},
    {20, "ENOTDIR"},
    {21, "EISDIR"},
    {22, "EINVAL"},
    {23, "ENFILE"},
    {24, "EMFILE"},
    {25, "ENOTTY"},
    {26, "ETXTBSY"},
    {27, "EFBIG"},
    {28, "ENOSPC"},
    {29, "ESPIPE"},
    {30, "EROFS"},
    {31, "EMLINK"},
    {32, "EPIPE"},
    {33, "EDOM"},
    {34, "ERANGE"},
    {35, "EDEADLK"},
    {36, "ENAMETOOLONG"},
    {37, "ENOLCK"},
    {38, "ENOSYS"},
    {39, "ENOTEMPTY"},
    {40, "ELOOP"},
    {42, "ENOMSG"},
    {43, "EIDRM"},
    {44, "ECHRNG"},
    {45, "EL2NSYNC"},
    {46, "EL3HLT"},
    {47, "EL3RST"},
    {48, "ELNRNG"},
    {49, "EUNATCH"},
    {50, "ENOCSI"},
    {51, "EL2HLT"},
    {52, "EBADE"},
    {53, "EBADR"},
    {54, "EXFULL"},
    {55, "ENOANO"},
    {56, "EBADRQC"},
    {57, "EBADSLT"},
    {59, "EBFONT"},
    {60, "ENOSTR"},
    {61, "ENODATA"},
    {62, "ETIME"},
    {63, "ENOSR"},
    {64, "ENONET"},
    {65, "ENOPKG"},
    {66, "EREMOTE"},
    {67, "ENOLINK"},
    {68, "EADV"},
    {69, "ESRMNT"},
    {70, "ECOMM"},
    {71, "EPROTO"},
    {72, "EMULTIHOP"},
    {73, "EDOTDOT"},
    {74, "EBADMSG"},
    {75, "EOVERFLOW"},
    {76, "ENOTUNIQ"},
    {77, "EBADFD"},
    {78, "EREMCHG"},
    {79, "ELIBACC"},
    {80, "ELIBBAD"},
    {81, "ELIBSCN"},
    {82, "ELIBMAX"},
    {83, "ELIBEXEC"},
    {84, "EILSEQ"},
    {85, "ERESTART"},
    {86, "ESTRPIPE"},
    {87, "EUSERS"},
    {88, "ENOTSOCK"},
    {89, "EDESTADDRREQ"},
    {90, "EMSGSIZE"},
    {91, "EPROTOTYPE"},
    {92, "ENOPROTOOPT"},
    {93, "EPROTONOSUPPORT"},
    {94, "ESOCKTNOSUPPORT"},
    {95, "EOPNOTSUPP"},
    {96, "EPFNOSUPPORT"},
    {97, "EAFNOSUPPORT"},
    {98, "EADDRINUSE"},
    {99, "EADDRNOTAVAIL"},
    {100, "ENETDOWN"},
    {101, "ENETUNREACH"},
    {102, "ENETRESET"},
    {103, "ECONNABORTED"},
    {104, "ECONNRESET"},
    {105, "ENOBUFS"},
    {106, "EISCONN"},
    {107, "ENOTCONN"},
    {108, "ESHUTDOWN"},
    {109, "ETOOMANYREFS"},
    {110, "ETIMEDOUT"},
    {111, "ECONNREFUSED"},
    {112, "EHOSTDOWN"},
    {113, "EHOSTUNREACH"},
    {114, "EALREADY"},
    {115, "EINPROGRESS"},
    {116, "ESTALE"},
    {117, "EUCLEAN"},
    {118, "ENOTNAM"},
    {119, "ENAVAIL"},
    {120, "EISNAM"},
    {121, "EREMOTEIO"},
    {122, "EDQUOT"},
    {123, "ENOMEDIUM"},
    {124, "EMEDIUMTYPE"},
    {125, "ECANCELED"},
    {126, "ENOKEY"},
    {127, "EKEYEXPIRED"},
    {128, "EKEYREVOKED"},
    {129, "EKEYREJECTED"},
    {130, "EOWNERDEAD"},
    {131, "ENOTRECOVERABLE"},
};

// Base types declaration
std::shared_ptr<ArionErrCodeType> ARION_ERR_CODE_TYPE;
std::shared_ptr<ArionFileDescriptorType> ARION_FILE_DESCRIPTOR_TYPE;
std::shared_ptr<ArionAccessModeType> ARION_ACCESS_MODE_TYPE;
std::shared_ptr<ArionOpenModeType> ARION_OPEN_MODE_TYPE;
std::shared_ptr<ArionFileATFlagType> ARION_FILE_AT_FLAG_TYPE;
std::shared_ptr<ArionStatxMaskType> ARION_STATX_MASK_TYPE;
std::shared_ptr<ArionStatxAttrsType> ARION_STATX_ATTRS_TYPE;
std::shared_ptr<ArionProtFlagType> ARION_PROT_FLAG_TYPE;
std::shared_ptr<ArionMmapFlagType> ARION_MMAP_FLAG_TYPE;
std::shared_ptr<ArionSeekWhenceType> ARION_SEEK_WHENCE_TYPE;
std::shared_ptr<ArionFutexOpType> ARION_FUTEX_OP_TYPE;
std::shared_ptr<ArionCloneFlagType> ARION_CLONE_FLAG_TYPE;
std::shared_ptr<ArionSignalType> ARION_SIGNAL_TYPE;
std::shared_ptr<ArionFileModeType> ARION_FILE_MODE_TYPE;
std::shared_ptr<ArionSocketDomainType> ARION_SOCKET_DOMAIN_TYPE;
std::shared_ptr<ArionSocketTypeType> ARION_SOCKET_TYPE_TYPE;
std::shared_ptr<ArionInAddrTType> ARION_IN_ADDR_T_TYPE;
std::shared_ptr<ArionIn6AddrTType> ARION_IN6_ADDR_T_TYPE;

// Struct factories declaration
std::shared_ptr<StatStructFactory> arion_poly_struct::STAT_STRUCT_FACTORY;
std::shared_ptr<StatxStructFactory> arion_poly_struct::STATX_STRUCT_FACTORY;
std::shared_ptr<TimespecStructFactory> arion_poly_struct::TIMESPEC_STRUCT_FACTORY;
std::shared_ptr<CloneArgsStructFactory> arion_poly_struct::CLONE_ARGS_STRUCT_FACTORY;
std::shared_ptr<SockaddrInStructFactory> arion_poly_struct::SOCKADDR_IN_STRUCT_FACTORY;
std::shared_ptr<SockaddrIn6StructFactory> arion_poly_struct::SOCKADDR_IN6_STRUCT_FACTORY;
std::shared_ptr<SockaddrUnStructFactory> arion_poly_struct::SOCKADDR_UN_STRUCT_FACTORY;

// Struct types declaration
std::shared_ptr<ArionStructStatType> arion_poly_struct::ARION_STRUCT_STAT_TYPE;
std::shared_ptr<ArionStructStatxType> arion_poly_struct::ARION_STRUCT_STATX_TYPE;
std::shared_ptr<ArionStructTimespecType> arion_poly_struct::ARION_STRUCT_TIMESPEC_TYPE;
std::shared_ptr<ArionStructCloneArgsType> arion_poly_struct::ARION_STRUCT_CLONE_ARGS_TYPE;
std::shared_ptr<ArionStructSockaddrInType> arion_poly_struct::ARION_STRUCT_SOCKADDR_IN_TYPE;
std::shared_ptr<ArionStructSockaddrIn6Type> arion_poly_struct::ARION_STRUCT_SOCKADDR_IN6_TYPE;
std::shared_ptr<ArionStructSockaddrUnType> arion_poly_struct::ARION_STRUCT_SOCKADDR_UN_TYPE;

// Variable struct types declaration
std::shared_ptr<ArionStructSockaddrType> arion_poly_struct::ARION_STRUCT_SOCKADDR_TYPE;

// Base types registration
REGISTER_ARION_TYPE(ARION_ERR_CODE_TYPE, ArionErrCodeType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_FILE_DESCRIPTOR_TYPE, ArionFileDescriptorType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_ACCESS_MODE_TYPE, ArionAccessModeType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_OPEN_MODE_TYPE, ArionOpenModeType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_FILE_AT_FLAG_TYPE, ArionFileATFlagType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STATX_MASK_TYPE, ArionStatxMaskType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STATX_ATTRS_TYPE, ArionStatxAttrsType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_PROT_FLAG_TYPE, ArionProtFlagType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_MMAP_FLAG_TYPE, ArionMmapFlagType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_SEEK_WHENCE_TYPE, ArionSeekWhenceType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_FUTEX_OP_TYPE, ArionFutexOpType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_CLONE_FLAG_TYPE, ArionCloneFlagType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_SIGNAL_TYPE, ArionSignalType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_FILE_MODE_TYPE, ArionFileModeType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_SOCKET_DOMAIN_TYPE, ArionSocketDomainType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_SOCKET_TYPE_TYPE, ArionSocketTypeType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_IN_ADDR_T_TYPE, ArionInAddrTType, OS_BASE_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_IN6_ADDR_T_TYPE, ArionIn6AddrTType, OS_BASE_TYPE_PRIORITY);

// Struct factories registration
REGISTER_ARION_TYPE(STAT_STRUCT_FACTORY, StatStructFactory, OS_STRUCT_FACTORY_PRIORITY);
REGISTER_ARION_TYPE(STATX_STRUCT_FACTORY, StatxStructFactory, OS_STRUCT_FACTORY_PRIORITY);
REGISTER_ARION_TYPE(TIMESPEC_STRUCT_FACTORY, TimespecStructFactory, OS_STRUCT_FACTORY_PRIORITY);
REGISTER_ARION_TYPE(CLONE_ARGS_STRUCT_FACTORY, CloneArgsStructFactory, OS_STRUCT_FACTORY_PRIORITY);
REGISTER_ARION_TYPE(SOCKADDR_IN_STRUCT_FACTORY, SockaddrInStructFactory, OS_STRUCT_FACTORY_PRIORITY);
REGISTER_ARION_TYPE(SOCKADDR_IN6_STRUCT_FACTORY, SockaddrIn6StructFactory, OS_STRUCT_FACTORY_PRIORITY);
REGISTER_ARION_TYPE(SOCKADDR_UN_STRUCT_FACTORY, SockaddrUnStructFactory, OS_STRUCT_FACTORY_PRIORITY);

// Struct types registration
REGISTER_ARION_TYPE(ARION_STRUCT_STAT_TYPE, ArionStructStatType, OS_STRUCT_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STRUCT_STATX_TYPE, ArionStructStatxType, OS_STRUCT_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STRUCT_TIMESPEC_TYPE, ArionStructTimespecType, OS_STRUCT_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STRUCT_CLONE_ARGS_TYPE, ArionStructCloneArgsType, OS_STRUCT_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STRUCT_SOCKADDR_IN_TYPE, ArionStructSockaddrInType, OS_STRUCT_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STRUCT_SOCKADDR_IN6_TYPE, ArionStructSockaddrIn6Type, OS_STRUCT_TYPE_PRIORITY);
REGISTER_ARION_TYPE(ARION_STRUCT_SOCKADDR_UN_TYPE, ArionStructSockaddrUnType, OS_STRUCT_TYPE_PRIORITY);

// Variable struct types registration
REGISTER_ARION_TYPE(ARION_STRUCT_SOCKADDR_TYPE, ArionStructSockaddrType, OS_VARIABLE_STRUCT_TYPE_PRIORITY);

std::string ArionErrCodeType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    auto flag_it = errno_names.find(-((int64_t)val));
    if (flag_it == errno_names.end())
        return int_to_hex<uint64_t>(val);
    return flag_it->second;
}

std::string ArionFileDescriptorType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    if (val == (uint32_t)AT_FDCWD)
        return "AT_FDCWD";
    return int_to_hex<uint64_t>(val);
}

std::string ArionInAddrTType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    struct in_addr ip_addr;
    ip_addr.s_addr = val;
    return std::string(inet_ntoa(ip_addr));
}

std::string ArionIn6AddrTType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    if (!arion->mem->is_mapped(val))
        return ARION_INT_TYPE->str(arion, val);
    std::vector<BYTE> ip_data = arion->mem->read(val, INET6_ADDRSTRLEN);
    char ip_str[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, ip_data.data(), ip_str, INET6_ADDRSTRLEN))
        return ARION_INT_TYPE->str(arion, val);
    return std::string(ip_str);
}

std::string ArionProtFlagType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    if (!val)
        return "PROT_NONE";
    std::stringstream prot_ss;
    if (val & PROT_GROWSDOWN)
        prot_ss << "GDOWN-";
    if (val & PROT_GROWSUP)
        prot_ss << "GUP-";
    if (val & PROT_READ)
        prot_ss << "R";
    else
        prot_ss << "-";
    if (val & PROT_WRITE)
        prot_ss << "W";
    else
        prot_ss << "-";
    if (val & PROT_EXEC)
        prot_ss << "X";
    else
        prot_ss << "-";
    return prot_ss.str();
}

std::shared_ptr<AbsArionStructType> ArionStructSockaddrType::process(std::shared_ptr<Arion> arion, uint64_t val)
{
    if (!arion->mem->is_mapped(val))
        return std::dynamic_pointer_cast<AbsArionStructType>(ARION_STRUCT_SOCKADDR_UN_TYPE);
    uint16_t sa_family_t = arion->mem->read_val(val, 2);
    switch (sa_family_t)
    {
    case AF_INET:
        return std::dynamic_pointer_cast<AbsArionStructType>(ARION_STRUCT_SOCKADDR_IN_TYPE);
    case AF_INET6:
        return std::dynamic_pointer_cast<AbsArionStructType>(ARION_STRUCT_SOCKADDR_IN6_TYPE);
    case AF_LOCAL:
        return std::dynamic_pointer_cast<AbsArionStructType>(ARION_STRUCT_SOCKADDR_UN_TYPE);
    default:
        return std::dynamic_pointer_cast<AbsArionStructType>(ARION_STRUCT_SOCKADDR_UN_TYPE);
    }
}

std::string ArionStructSockaddrUnType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    arion::CPU_ARCH arch = this->arion_curr_arch(arion);
    std::string struct_str = ArionStructType::str(arion, val);
    if (!arion->mem->is_mapped(val + 2))
        return int_to_hex<uint64_t>(val);
    std::string sun_path = arion->mem->read_c_string(val + 2);
    struct_str = struct_str.substr(0, struct_str.length() - 1) + ", sun_path=\"" + sun_path + "\"}";
    return struct_str;
}

PROT_FLAGS kernel_prot_to_arion_prot(int kflags)
{
    PROT_FLAGS flags = 0;
    if (kflags & PROT_EXEC)
        flags |= 1;
    if (kflags & PROT_WRITE)
        flags |= 2;
    if (kflags & PROT_READ)
        flags |= 4;
    return flags;
}
