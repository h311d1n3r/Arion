#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <errno.h>
#include <sys/mman.h>

using namespace arion;

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

std::shared_ptr<arion_poly_struct::StatStructFactory> arion_poly_struct::STAT_STRUCT_FACTORY = std::make_shared<arion_poly_struct::StatStructFactory>();
std::shared_ptr<arion_poly_struct::StatxStructFactory> arion_poly_struct::STATX_STRUCT_FACTORY = std::make_shared<arion_poly_struct::StatxStructFactory>();

std::shared_ptr<ArionErrCodeType> ARION_ERR_CODE_TYPE = std::make_shared<ArionErrCodeType>();
std::shared_ptr<ArionFileDescriptorType> ARION_FILE_DESCRIPTOR_TYPE = std::make_shared<ArionFileDescriptorType>();
std::shared_ptr<ArionAccessModeType> ARION_ACCESS_MODE_TYPE = std::make_shared<ArionAccessModeType>();
std::shared_ptr<ArionOpenModeType> ARION_OPEN_MODE_TYPE = std::make_shared<ArionOpenModeType>();
std::shared_ptr<ArionStructStatType> ARION_STRUCT_STAT_TYPE = std::make_shared<ArionStructStatType>();

std::string ArionErrCodeType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    auto flag_it = errno_names.find(-((int64_t)val));
    if(flag_it == errno_names.end())
        return int_to_hex<uint64_t>(val);
    return flag_it->second;
}

std::string ArionFileDescriptorType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    if(val == (uint32_t)AT_FDCWD)
        return "AT_FDCWD";
    return int_to_hex<uint64_t>(val);
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