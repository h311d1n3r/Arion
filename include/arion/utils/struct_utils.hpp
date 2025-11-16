#ifndef ARION_STRUCT_UTILS_HPP
#define ARION_STRUCT_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/utils/host_utils.hpp>
#include <arion/utils/type_utils.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace arion_poly_struct
{
/// Type definition for a unique ID assigned to a managed polymorphic struct instance.
using STRUCT_ID = uint64_t;

/// Map defining the required memory alignment (in bytes) based on the CPU architecture.
inline std::map<arion::CPU_ARCH, uint8_t> ALIGN_BY_ARCH{
    {arion::CPU_ARCH::X86_ARCH, 4},
    {arion::CPU_ARCH::X8664_ARCH, 8},
    {arion::CPU_ARCH::ARM_ARCH, 8},
    {arion::CPU_ARCH::ARM64_ARCH, 8},
};

/// Map defining the size of a pointer (in bytes) based on the CPU architecture.
inline std::map<arion::CPU_ARCH, uint8_t> PTR_SZ_BY_ARCH{
    {arion::CPU_ARCH::X86_ARCH, 4},
    {arion::CPU_ARCH::X8664_ARCH, 8},
    {arion::CPU_ARCH::ARM_ARCH, 4},
    {arion::CPU_ARCH::ARM64_ARCH, 8},
};

/// Map defining the architecture size (in bits) based on the CPU architecture.
inline std::map<arion::CPU_ARCH, uint16_t> ARCH_SZ{
    {arion::CPU_ARCH::X86_ARCH, 32},
    {arion::CPU_ARCH::X8664_ARCH, 64},
    {arion::CPU_ARCH::ARM_ARCH, 32},
    {arion::CPU_ARCH::ARM64_ARCH, 64},
};

/// Enumeration for specifying the size and type of a polymorphic structure field.
enum POLYMORPHIC_STRUCT_FIELD_TYPE
{
    /// 8-bit integer type.
    V8,
    /// 16-bit integer type.
    V16,
    /// 32-bit integer type.
    V32,
    /// 64-bit integer type.
    V64,
    /// Pointer size type (size determined by architecture).
    PTR_SZ,
    /// 8-bit array type.
    A8,
    /// 16-bit array type.
    A16,
    /// 32-bit array type.
    A32,
    /// 64-bit array type.
    A64,
    /// Pointer size array type (array of pointers).
    PTR_ARR
};

/// Structure defining an architecture constraint for a field in a polymorphic struct.
struct POLYMORPHIC_STRUCT_CONSTRAINT
{
    /// The specific architecture required for the field to be present/used.
    arion::CPU_ARCH arch;
    /// The specific architecture bit size required (e.g., 32 or 64 bit).
    uint16_t arch_sz;

    /**
     * Builder for an unconstrained POLYMORPHIC_STRUCT_CONSTRAINT instance.
     */
    POLYMORPHIC_STRUCT_CONSTRAINT() : arch(arion::CPU_ARCH::UNKNOWN_ARCH), arch_sz(0) {};
    /**
     * Builder for a constraint based on architecture.
     * @param[in] arch The specific CPU architecture.
     */
    POLYMORPHIC_STRUCT_CONSTRAINT(arion::CPU_ARCH arch) : arch(arch), arch_sz(0) {};
    /**
     * Builder for a constraint based on architecture size.
     * @param[in] arch_sz The specific architecture size (32 or 64).
     */
    POLYMORPHIC_STRUCT_CONSTRAINT(uint16_t arch_sz) : arch(arion::CPU_ARCH::UNKNOWN_ARCH), arch_sz(arch_sz) {};
};

/// Structure defining a field within a polymorphic struct, including its constraints and type metadata.
struct POLYMORPHIC_STRUCT_FIELD
{
    /// Constraints dictating when this field is active.
    POLYMORPHIC_STRUCT_CONSTRAINT constraint;
    /// The type and size of the field.
    POLYMORPHIC_STRUCT_FIELD_TYPE type;
    /// The Arion type object used for displaying/interpreting the field value.
    std::shared_ptr<arion_type::KernelType> arion_type;
    /// The name of the field within the structure.
    std::string name;
    /// Size of the array (if an array type, otherwise 0).
    size_t sz;

    /**
     * Builder for POLYMORPHIC_STRUCT_FIELD instances with full parameters.
     */
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_CONSTRAINT constraint, POLYMORPHIC_STRUCT_FIELD_TYPE type,
                             std::shared_ptr<arion_type::KernelType> arion_type, std::string name, size_t sz)
        : constraint(constraint), type(type), arion_type(arion_type), name(name), sz(sz) {};
    /**
     * Builder for non-array POLYMORPHIC_STRUCT_FIELD instances with constraints.
     */
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_CONSTRAINT constraint, POLYMORPHIC_STRUCT_FIELD_TYPE type,
                             std::shared_ptr<arion_type::KernelType> arion_type, std::string name)
        : POLYMORPHIC_STRUCT_FIELD(constraint, type, arion_type, name, 0) {};
    /**
     * Builder for POLYMORPHIC_STRUCT_FIELD instances with array size but no constraints.
     */
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_FIELD_TYPE type, std::shared_ptr<arion_type::KernelType> arion_type,
                             std::string name, size_t sz)
        : POLYMORPHIC_STRUCT_FIELD({}, type, arion_type, name, sz) {};
    /**
     * Builder for non-array POLYMORPHIC_STRUCT_FIELD instances with no constraints.
     */
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_FIELD_TYPE type, std::shared_ptr<arion_type::KernelType> arion_type,
                             std::string name)
        : POLYMORPHIC_STRUCT_FIELD({}, type, arion_type, name, 0) {};
};

/// Represents a single instance of a polymorphic structure, storing its field values dynamically.
class PolymorphicStruct
{
  private:
    /// Map storing integer/primitive field values by name.
    std::map<std::string, uint64_t> int_vals;
    /// Map storing pointers to dynamically allocated array field values by name.
    std::map<std::string, void *> arr_vals;

  public:
    /**
     * Destructor: handles the cleanup of dynamically allocated array memory.
     */
    ~PolymorphicStruct()
    {
        for (auto arr_it : this->arr_vals)
            free(arr_it.second);
    }
    /// Sets an integer/primitive field value.
    void set_field(std::string name, uint64_t val)
    {
        this->int_vals[name] = val;
    }
    /// Sets an array field value (takes ownership of the pointer).
    void set_field(std::string name, void *arr)
    {
        auto arr_it = this->arr_vals.find(name);
        if (arr_it != this->arr_vals.end())
            free(arr_it->second);
        this->arr_vals[name] = arr;
    }
    /// Checks if a named integer field exists.
    bool has_int_field(std::string name)
    {
        auto int_it = this->int_vals.find(name);
        return int_it != this->int_vals.end();
    }
    /// Retrieves the value of a named integer field.
    uint64_t get_int_field(std::string name)
    {
        auto int_it = this->int_vals.find(name);
        if (int_it == this->int_vals.end())
            throw arion_exception::NoStructFieldWithNameException(name);
        return int_it->second;
    }
    /// Checks if a named array field exists.
    bool has_arr_field(std::string name)
    {
        auto arr_it = this->arr_vals.find(name);
        return arr_it != this->arr_vals.end();
    }
    /// Retrieves the pointer to a named array field.
    void *get_arr_field(std::string name)
    {
        auto arr_it = this->arr_vals.find(name);
        if (arr_it == this->arr_vals.end())
            throw arion_exception::NoStructFieldWithNameException(name);
        return arr_it->second;
    }
};

/**
 * Manages the definition and instantiation of polymorphic structures (structs whose layout changes per architecture).
 * @tparam T The host-side C++ type corresponding to the structure being managed.
 */
template <typename T> class PolymorphicStructFactory
{
  private:
    /// Counter for generating unique structure IDs.
    STRUCT_ID curr_id = 1;
    /// Stack of released IDs for reuse.
    std::stack<STRUCT_ID> free_ids;
    /// Map of currently allocated polymorphic structure instances.
    std::map<STRUCT_ID, std::unique_ptr<PolymorphicStruct>> curr_structs;
    /// Definition of all potential fields in the polymorphic structure.
    std::vector<POLYMORPHIC_STRUCT_FIELD> fields;

    /**
     * Generates a new unique `STRUCT_ID`.
     * @return The next available `STRUCT_ID`.
     */
    STRUCT_ID gen_next_id()
    {
        STRUCT_ID id;
        if (!this->free_ids.empty())
        {
            id = this->free_ids.top();
            this->free_ids.pop();
        }
        else
        {
            if (this->curr_id == ARION_MAX_U64)
                throw arion_exception::TooManyStructsException();
            id = this->curr_id++;
        }
        return id;
    }

    /**
     * Allocates a new `STRUCT_ID` and stores the polymorphic instance.
     * @param[in] struct_inst Unique pointer to the `PolymorphicStruct` instance.
     * @return The assigned `STRUCT_ID`.
     */
    STRUCT_ID add_struct_entry(std::unique_ptr<PolymorphicStruct> struct_inst)
    {
        STRUCT_ID id = this->gen_next_id();
        this->curr_structs[id] = std::move(struct_inst);
        return id;
    }

    /**
     * Retrieves the required memory alignment for the given architecture.
     * @param[in] arch The CPU architecture.
     * @return The alignment size in bytes.
     */
    uint8_t get_align(arion::CPU_ARCH arch)
    {
        auto align_it = ALIGN_BY_ARCH.find(arch);
        if (align_it == ALIGN_BY_ARCH.end())
            throw arion_exception::UnsupportedCpuArchException();
        return align_it->second;
    }

    /**
     * Retrieves the pointer size for the given architecture.
     * @param[in] arch The CPU architecture.
     * @return The pointer size in bytes.
     */
    uint8_t get_ptr_sz(arion::CPU_ARCH arch)
    {
        auto ptr_sz_it = PTR_SZ_BY_ARCH.find(arch);
        if (ptr_sz_it == PTR_SZ_BY_ARCH.end())
            throw arion_exception::UnsupportedCpuArchException();
        return ptr_sz_it->second;
    }

    /**
     * Retrieves the bit size (32 or 64) for the given architecture.
     * @param[in] arch The CPU architecture.
     * @return The architecture size in bits.
     */
    uint16_t get_arch_sz(arion::CPU_ARCH arch)
    {
        auto arch_sz_it = ARCH_SZ.find(arch);
        if (arch_sz_it == ARCH_SZ.end())
            throw arion_exception::UnsupportedCpuArchException();
        return arch_sz_it->second;
    }

  protected:
    /**
     * Builder for PolymorphicStructFactory instances.
     * @param[in] fields The vector of field definitions defining the structure layout.
     */
    PolymorphicStructFactory(std::vector<POLYMORPHIC_STRUCT_FIELD> fields) : fields(std::move(fields)) {};
    /**
     * Destructor: cleans up all currently managed structure instances.
     */
    ~PolymorphicStructFactory()
    {
        this->clear_structs();
    }

    /// Macro for easy access to `POLYMORPHIC_STRUCT_FIELD_TYPE` in subclass definitions.
    using ftype = arion_poly_struct::POLYMORPHIC_STRUCT_FIELD_TYPE;

  public:
    /**
     * Calculates the memory size of the structure instance for a given target architecture, accounting for padding and
     * alignment.
     * @param[in] arch The target CPU architecture.
     * @return The calculated size of the structure in bytes.
     */
    size_t get_struct_sz(arion::CPU_ARCH arch)
    {
        size_t struct_sz = 0;
        uint8_t align = this->get_align(arch);
        uint8_t ptr_sz = this->get_ptr_sz(arch);
        uint16_t arch_sz = this->get_arch_sz(arch);
        for (POLYMORPHIC_STRUCT_FIELD field : this->fields)
        {
            if (field.constraint.arch != arion::CPU_ARCH::UNKNOWN_ARCH && arch != field.constraint.arch)
                continue;
            if (field.constraint.arch_sz != 0 && arch_sz != field.constraint.arch_sz)
                continue;

            switch (field.type)
            {
            case V8: {
                uint8_t val;
                struct_sz += sizeof(uint8_t);
                break;
            }
            case V16: {
                if (struct_sz % sizeof(uint16_t))
                    struct_sz += sizeof(uint16_t) - (struct_sz % sizeof(uint16_t));
                struct_sz += sizeof(uint16_t);
                break;
            }
            case V32: {
                if (struct_sz % sizeof(uint32_t))
                    struct_sz += sizeof(uint32_t) - (struct_sz % sizeof(uint32_t));
                struct_sz += sizeof(uint32_t);
                break;
            }
            case V64: {
                if (struct_sz % sizeof(uint64_t))
                    struct_sz += sizeof(uint64_t) - (struct_sz % sizeof(uint64_t));
                struct_sz += sizeof(uint64_t);
                break;
            }
            case PTR_SZ: {
                if (struct_sz % ptr_sz)
                    struct_sz += ptr_sz - (struct_sz % ptr_sz);
                struct_sz += ptr_sz;
                break;
            }
            case A8: {
                size_t arr_sz = field.sz * sizeof(uint8_t);
                struct_sz += arr_sz;
                break;
            }
            case A16: {
                if (struct_sz % sizeof(uint16_t))
                    struct_sz += sizeof(uint16_t) - (struct_sz % sizeof(uint16_t));
                size_t arr_sz = field.sz * sizeof(uint16_t);
                struct_sz += arr_sz;
                break;
            }
            case A32: {
                if (struct_sz % sizeof(uint32_t))
                    struct_sz += sizeof(uint32_t) - (struct_sz % sizeof(uint32_t));
                size_t arr_sz = field.sz * sizeof(uint32_t);
                struct_sz += arr_sz;
                break;
            }
            case A64: {
                if (struct_sz % sizeof(uint64_t))
                    struct_sz += sizeof(uint64_t) - (struct_sz % sizeof(uint64_t));
                size_t arr_sz = field.sz * sizeof(uint64_t);
                struct_sz += arr_sz;
                break;
            }
            case PTR_ARR: {
                if (struct_sz % ptr_sz)
                    struct_sz += ptr_sz - (struct_sz % ptr_sz);
                size_t arr_sz = field.sz * ptr_sz;
                struct_sz += arr_sz;
                break;
            }
            default:
                break;
            }
        }
        if (struct_sz % align)
        {
            uint8_t align_sz = align - (struct_sz % align);
            struct_sz += align_sz;
        }
        return struct_sz;
    }

    /**
     * Calculates the memory size of the structure instance for the host architecture.
     * @return The calculated size of the structure in bytes.
     */
    size_t get_host_struct_sz()
    {
        return this->get_struct_sz(arion::get_host_cpu_arch());
    }

    /**
     * Parses raw memory data representing a structure instance and creates a corresponding `PolymorphicStruct` object.
     * @param[in] arch The architecture used to interpret the structure layout (size, alignment).
     * @param[in] struct_inst Pointer to the raw byte data of the structure.
     * @return The unique ID of the newly created `PolymorphicStruct` instance.
     */
    STRUCT_ID feed(arion::CPU_ARCH arch, arion::BYTE *struct_inst)
    {
        uint8_t align = this->get_align(arch);
        uint8_t ptr_sz = this->get_ptr_sz(arch);
        uint16_t arch_sz = this->get_arch_sz(arch);
        std::unique_ptr<PolymorphicStruct> curr_struct = std::make_unique<PolymorphicStruct>();

        off_t off = 0;
        for (POLYMORPHIC_STRUCT_FIELD field : this->fields)
        {
            if (field.constraint.arch != arion::CPU_ARCH::UNKNOWN_ARCH && arch != field.constraint.arch)
                continue;
            if (field.constraint.arch_sz != 0 && arch_sz != field.constraint.arch_sz)
                continue;

            switch (field.type)
            {
            case V8: {
                uint8_t val;
                memcpy(&val, struct_inst + off, sizeof(uint8_t));
                curr_struct->set_field(field.name, val);
                off += sizeof(uint8_t);
                break;
            }
            case V16: {
                if (off % sizeof(uint16_t))
                    off += sizeof(uint16_t) - (off % sizeof(uint16_t));
                uint16_t val;
                memcpy(&val, struct_inst + off, sizeof(uint16_t));
                curr_struct->set_field(field.name, val);
                off += sizeof(uint16_t);
                break;
            }
            case V32: {
                if (off % sizeof(uint32_t))
                    off += sizeof(uint32_t) - (off % sizeof(uint32_t));
                uint32_t val;
                memcpy(&val, struct_inst + off, sizeof(uint32_t));
                curr_struct->set_field(field.name, val);
                off += sizeof(uint32_t);
                break;
            }
            case V64: {
                if (off % sizeof(uint64_t))
                    off += sizeof(uint64_t) - (off % sizeof(uint64_t));
                uint64_t val;
                memcpy(&val, struct_inst + off, sizeof(uint64_t));
                curr_struct->set_field(field.name, val);
                off += sizeof(uint64_t);
                break;
            }
            case PTR_SZ: {
                if (off % ptr_sz)
                    off += ptr_sz - (off % ptr_sz);
                uint64_t val;
                memcpy(&val, struct_inst + off, ptr_sz);
                curr_struct->set_field(field.name, val);
                off += ptr_sz;
                break;
            }
            case A8: {
                size_t arr_sz = field.sz * sizeof(uint8_t);
                uint8_t *arr = (uint8_t *)malloc(arr_sz);
                memcpy(arr, struct_inst + off, arr_sz);
                curr_struct->set_field(field.name, arr);
                off += arr_sz;
                break;
            }
            case A16: {
                if (off % sizeof(uint16_t))
                    off += sizeof(uint16_t) - (off % sizeof(uint16_t));
                size_t arr_sz = field.sz * sizeof(uint16_t);
                uint16_t *arr = (uint16_t *)malloc(arr_sz);
                memcpy(arr, struct_inst + off, arr_sz);
                curr_struct->set_field(field.name, arr);
                off += arr_sz;
                break;
            }
            case A32: {
                if (off % sizeof(uint32_t))
                    off += sizeof(uint32_t) - (off % sizeof(uint32_t));
                size_t arr_sz = field.sz * sizeof(uint32_t);
                uint32_t *arr = (uint32_t *)malloc(arr_sz);
                memcpy(arr, struct_inst + off, arr_sz);
                curr_struct->set_field(field.name, arr);
                off += arr_sz;
                break;
            }
            case A64: {
                if (off % sizeof(uint64_t))
                    off += sizeof(uint64_t) - (off % sizeof(uint64_t));
                size_t arr_sz = field.sz * sizeof(uint64_t);
                uint64_t *arr = (uint64_t *)malloc(arr_sz);
                memcpy(arr, struct_inst + off, arr_sz);
                curr_struct->set_field(field.name, arr);
                off += arr_sz;
                break;
            }
            case PTR_ARR: {
                if (off % ptr_sz)
                    off += ptr_sz - (off % ptr_sz);
                size_t arr_sz = field.sz * ptr_sz;
                uint8_t *arr = (uint8_t *)malloc(arr_sz);
                memcpy(arr, struct_inst + off, arr_sz);
                curr_struct->set_field(field.name, arr);
                off += arr_sz;
                break;
            }
            default:
                break;
            }
        }
        STRUCT_ID id = this->add_struct_entry(std::move(curr_struct));
        return id;
    }

    /**
     * Feeds an existing host-side structure instance to the factory and gets its `STRUCT_ID`.
     * @param[in] struct_inst Pointer to the host structure instance.
     * @return The unique ID of the newly created `PolymorphicStruct` instance.
     */
    STRUCT_ID feed_host(T *struct_inst)
    {
        arion::BYTE *data = (arion::BYTE *)malloc(sizeof(T));
        memcpy(data, struct_inst, sizeof(T));
        STRUCT_ID id = this->feed(arion::get_host_cpu_arch(), data);
        free(data);
        return id;
    }

    /**
     * Builds the raw memory representation of a managed structure instance for a target architecture.
     * @param[in] id The ID of the managed structure instance.
     * @param[in] arch The target CPU architecture for layout.
     * @param[out] struct_len The size of the resulting raw data buffer.
     * @return A pointer to the dynamically allocated raw byte array representing the structure.
     */
    arion::BYTE *build(STRUCT_ID id, arion::CPU_ARCH arch, size_t &struct_len)
    {
        auto struct_it = this->curr_structs.find(id);
        if (struct_it == this->curr_structs.end())
            throw arion_exception::WrongStructIdException();
        std::unique_ptr<PolymorphicStruct> curr_struct = std::move(struct_it->second);

        std::vector<arion::BYTE> data_vec;
        uint8_t align = this->get_align(arch);
        uint8_t ptr_sz = this->get_ptr_sz(arch);
        uint16_t arch_sz = this->get_arch_sz(arch);

        for (POLYMORPHIC_STRUCT_FIELD field : this->fields)
        {
            if (field.constraint.arch != arion::CPU_ARCH::UNKNOWN_ARCH && arch != field.constraint.arch)
                continue;
            if (field.constraint.arch_sz != 0 && arch_sz != field.constraint.arch_sz)
                continue;

            switch (field.type)
            {
            case V8: {
                uint8_t val = 0;
                if (curr_struct->has_int_field(field.name))
                    val = curr_struct->get_int_field(field.name);
                data_vec.push_back(val);
                break;
            }
            case V16: {
                if (data_vec.size() % sizeof(uint16_t))
                {
                    uint8_t align_sz = sizeof(uint16_t) - (data_vec.size() % sizeof(uint16_t));
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                uint16_t val = 0;
                if (curr_struct->has_int_field(field.name))
                    val = curr_struct->get_int_field(field.name);
                data_vec.insert(data_vec.end(), (uint8_t *)&val, (uint8_t *)&val + sizeof(uint16_t));
                break;
            }
            case V32: {
                if (data_vec.size() % sizeof(uint32_t))
                {
                    uint8_t align_sz = sizeof(uint32_t) - (data_vec.size() % sizeof(uint32_t));
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                uint32_t val = 0;
                if (curr_struct->has_int_field(field.name))
                    val = curr_struct->get_int_field(field.name);
                data_vec.insert(data_vec.end(), (uint8_t *)&val, (uint8_t *)&val + sizeof(uint32_t));
                break;
            }
            case V64: {
                if (data_vec.size() % sizeof(uint64_t))
                {
                    uint8_t align_sz = sizeof(uint64_t) - (data_vec.size() % sizeof(uint64_t));
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                uint64_t val = 0;
                if (curr_struct->has_int_field(field.name))
                    val = curr_struct->get_int_field(field.name);
                data_vec.insert(data_vec.end(), (uint8_t *)&val, (uint8_t *)&val + sizeof(uint64_t));
                break;
            }
            case PTR_SZ: {
                if (data_vec.size() % ptr_sz)
                {
                    uint8_t align_sz = ptr_sz - (data_vec.size() % ptr_sz);
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                uint64_t val = 0;
                if (curr_struct->has_int_field(field.name))
                    val = curr_struct->get_int_field(field.name);
                data_vec.insert(data_vec.end(), (uint8_t *)&val, (uint8_t *)&val + ptr_sz);
                break;
            }
            case A8: {
                size_t arr_sz = field.sz * sizeof(uint8_t);
                bool has_field = curr_struct->has_arr_field(field.name);
                uint8_t *arr;
                if (has_field)
                    arr = (uint8_t *)curr_struct->get_arr_field(field.name);
                else
                    arr = (uint8_t *)malloc(arr_sz);
                data_vec.insert(data_vec.end(), arr, arr + arr_sz);
                if (!has_field)
                    free(arr);
                break;
            }
            case A16: {
                if (data_vec.size() % sizeof(uint16_t))
                {
                    uint8_t align_sz = sizeof(uint16_t) - (data_vec.size() % sizeof(uint16_t));
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                size_t arr_sz = field.sz * sizeof(uint16_t);
                bool has_field = curr_struct->has_arr_field(field.name);
                uint16_t *arr;
                if (has_field)
                    arr = (uint16_t *)curr_struct->get_arr_field(field.name);
                else
                    arr = (uint16_t *)malloc(arr_sz);
                data_vec.insert(data_vec.end(), (uint8_t *)arr, (uint8_t *)arr + arr_sz);
                if (!has_field)
                    free(arr);
                break;
            }
            case A32: {
                if (data_vec.size() % sizeof(uint32_t))
                {
                    uint8_t align_sz = sizeof(uint32_t) - (data_vec.size() % sizeof(uint32_t));
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                size_t arr_sz = field.sz * sizeof(uint32_t);
                bool has_field = curr_struct->has_arr_field(field.name);
                uint32_t *arr;
                if (has_field)
                    arr = (uint32_t *)curr_struct->get_arr_field(field.name);
                else
                    arr = (uint32_t *)malloc(arr_sz);
                data_vec.insert(data_vec.end(), (uint8_t *)arr, (uint8_t *)arr + arr_sz);
                if (!has_field)
                    free(arr);
                break;
            }
            case A64: {
                if (data_vec.size() % sizeof(uint64_t))
                {
                    uint8_t align_sz = sizeof(uint64_t) - (data_vec.size() % sizeof(uint64_t));
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                size_t arr_sz = field.sz * sizeof(uint64_t);
                bool has_field = curr_struct->has_arr_field(field.name);
                uint64_t *arr;
                if (has_field)
                    arr = (uint64_t *)curr_struct->get_arr_field(field.name);
                else
                    arr = (uint64_t *)malloc(arr_sz);
                data_vec.insert(data_vec.end(), (uint8_t *)arr, (uint8_t *)arr + arr_sz);
                if (!has_field)
                    free(arr);
                break;
            }
            case PTR_ARR: {
                if (data_vec.size() % ptr_sz)
                {
                    uint8_t align_sz = ptr_sz - (data_vec.size() % ptr_sz);
                    data_vec.insert(data_vec.end(), align_sz, 0);
                }
                size_t arr_sz = field.sz * ptr_sz;
                bool has_field = curr_struct->has_arr_field(field.name);
                uint8_t *arr;
                if (has_field)
                    arr = (uint8_t *)curr_struct->get_arr_field(field.name);
                else
                    arr = (uint8_t *)malloc(arr_sz);
                data_vec.insert(data_vec.end(), (uint8_t *)arr, (uint8_t *)arr + arr_sz);
                if (!has_field)
                    free(arr);
                break;
            }
            default:
                break;
            }
        }

        if (data_vec.size() % align)
        {
            uint8_t align_sz = align - (data_vec.size() % align);
            data_vec.insert(data_vec.end(), align_sz, 0);
        }
        struct_len = data_vec.size();
        arion::BYTE *data = (arion::BYTE *)malloc(struct_len);
        memcpy(data, data_vec.data(), struct_len);
        this->curr_structs[id] = std::move(curr_struct);
        return data;
    }

    /**
     * Builds and returns a host-side C++ structure (`T`) instance from a managed instance.
     * @param[in] id The ID of the managed structure instance.
     * @return A pointer to the newly allocated host structure (`T*`).
     */
    T *build_host(STRUCT_ID id)
    {
        size_t data_len;
        arion::BYTE *data = this->build(id, arion::get_host_cpu_arch(), data_len);
        T *struct_inst = (T *)malloc(sizeof(T));
        size_t min_sz = std::min(sizeof(T), data_len);
        memcpy(struct_inst, data, min_sz);
        free(data);
        return struct_inst;
    }

    /**
     * Generates a string representation of the managed structure instance, formatting values using their `KernelType`.
     * @param[in] id The ID of the managed structure instance.
     * @param[in] arion Shared pointer to the Arion instance (needed for memory access in some `KernelType::str` calls).
     * @param[in] arch The architecture to assume for pointer sizes and field interpretation.
     * @return A formatted string representation of the structure (e.g., "{field1=value, field2=value}").
     */
    std::string to_string(STRUCT_ID id, std::shared_ptr<arion::Arion> arion, arion::CPU_ARCH arch)
    {
        auto struct_it = this->curr_structs.find(id);
        if (struct_it == this->curr_structs.end())
            throw arion_exception::WrongStructIdException();
        std::unique_ptr<PolymorphicStruct> curr_struct = std::move(struct_it->second);

        uint8_t align = this->get_align(arch);
        uint8_t ptr_sz = this->get_ptr_sz(arch);
        uint16_t arch_sz = this->get_arch_sz(arch);

        std::stringstream ss;
        ss << "{";

        for (POLYMORPHIC_STRUCT_FIELD field : this->fields)
        {
            if (field.constraint.arch != arion::CPU_ARCH::UNKNOWN_ARCH && arch != field.constraint.arch)
                continue;
            if (field.constraint.arch_sz != 0 && arch_sz != field.constraint.arch_sz)
                continue;

            if (ss.str().size() > 1)
                ss << ", ";
            ss << field.name << "=";
            switch (field.type)
            {
            case V8:
            case V16:
            case V32:
            case V64:
            case PTR_SZ: {
                uint64_t val = 0;
                if (curr_struct->has_int_field(field.name))
                    val = curr_struct->get_int_field(field.name);
                ss << field.arion_type->str(arion, val);
                break;
            }
            case A8: {
                ss << "{";
                if (curr_struct->has_arr_field(field.name))
                {
                    uint8_t *arr = (uint8_t *)curr_struct->get_arr_field(field.name);
                    for (size_t arr_i = 0; arr_i < field.sz; arr_i++)
                    {
                        ss << field.arion_type->str(arion, arr[arr_i]);
                        if (arr_i < field.sz - 1)
                            ss << ", ";
                    }
                }
                ss << "}";
                break;
            }
            case A16: {
                ss << "{";
                if (curr_struct->has_arr_field(field.name))
                {
                    uint16_t *arr = (uint16_t *)curr_struct->get_arr_field(field.name);
                    for (size_t arr_i = 0; arr_i < field.sz; arr_i++)
                    {
                        ss << field.arion_type->str(arion, arr[arr_i]);
                        if (arr_i < field.sz - 1)
                            ss << ", ";
                    }
                }
                ss << "}";
                break;
            }
            case A32: {
                ss << "{";
                if (curr_struct->has_arr_field(field.name))
                {
                    uint32_t *arr = (uint32_t *)curr_struct->get_arr_field(field.name);
                    for (size_t arr_i = 0; arr_i < field.sz; arr_i++)
                    {
                        ss << field.arion_type->str(arion, arr[arr_i]);
                        if (arr_i < field.sz - 1)
                            ss << ", ";
                    }
                }
                ss << "}";
                break;
            }
            case A64: {
                ss << "{";
                if (curr_struct->has_arr_field(field.name))
                {
                    uint64_t *arr = (uint64_t *)curr_struct->get_arr_field(field.name);
                    for (size_t arr_i = 0; arr_i < field.sz; arr_i++)
                    {
                        ss << field.arion_type->str(arion, arr[arr_i]);
                        if (arr_i < field.sz - 1)
                            ss << ", ";
                    }
                }
                ss << "}";
                break;
            }
            case PTR_ARR: {
                ss << "{";
                if (curr_struct->has_arr_field(field.name))
                {
                    void *arr = curr_struct->get_arr_field(field.name);
                    for (size_t arr_i = 0; arr_i < field.sz; arr_i++)
                    {
                        uint64_t val = 0;
                        memcpy(&val, (void *)((uint64_t)arr + arr_i * ptr_sz), ptr_sz);
                        ss << field.arion_type->str(arion, val);
                        if (arr_i < field.sz - 1)
                            ss << ", ";
                    }
                }
                ss << "}";
                break;
            }
            default:
                ss << "UNK_FIELD";
                break;
            }
        }

        ss << "}";

        // Move the unique_ptr back into the map
        this->curr_structs[id] = std::move(curr_struct);
        return ss.str();
    }

    /**
     * Releases a managed structure instance, freeing its allocated resources and making its ID available for reuse.
     * @param[in] id The ID of the structure instance to release.
     */
    void release_struct(STRUCT_ID id)
    {
        auto struct_it = this->curr_structs.find(id);
        if (struct_it == this->curr_structs.end())
            throw arion_exception::WrongStructIdException();

        this->curr_structs.erase(id);
        if (this->curr_structs.size())
            this->free_ids.push(id);
        else
        {
            this->free_ids = std::stack<STRUCT_ID>();
            this->curr_id = 1;
        }
    }

    /**
     * Releases all currently managed structure instances.
     */
    void clear_structs()
    {
        std::vector<STRUCT_ID> struct_ids; // need to clone keys to prevent concurrency editing
        for (const auto &struct_pair : this->curr_structs)
            struct_ids.push_back(struct_pair.first);
        for (STRUCT_ID struct_id : struct_ids)
            this->release_struct(struct_id);
    }
};

/// Abstract base class providing common utilities for structure types that interact with emulated memory.
class AbsArionStructType
{
  protected:
    /**
     * Protected destructor to ensure deletion only via smart pointers or derived classes.
     */
    virtual ~AbsArionStructType() = default;
    /**
     * Checks if a memory address is mapped within the emulated memory.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] addr The memory address to check.
     * @return `true` if the memory is mapped, `false` otherwise.
     */
    bool arion_is_mapped(std::shared_ptr<arion::Arion> arion, arion::ADDR addr);
    /**
     * Retrieves the current architecture of the emulated CPU.
     * @param[in] arion Shared pointer to the Arion instance.
     * @return The `CPU_ARCH` of the emulated system.
     */
    arion::CPU_ARCH arion_curr_arch(std::shared_ptr<arion::Arion> arion);
    /**
     * Reads a chunk of memory from the emulated system.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] addr The starting address to read from.
     * @param[in] sz The number of bytes to read.
     * @return A vector of bytes containing the read memory data.
     */
    std::vector<arion::BYTE> arion_read_mem(std::shared_ptr<arion::Arion> arion, arion::ADDR addr, size_t sz);
};

/**
 * Represents a standard, polymorphic structure type (fixed layout, but size/field interpretation depends on
 * architecture).
 * @tparam T The host-side C++ type corresponding to the structure being represented.
 */
template <typename T> class ArionStructType : public arion_type::KernelType, public AbsArionStructType
{
  private:
    /// The factory responsible for handling the polymorphic nature of the structure.
    std::shared_ptr<arion_poly_struct::PolymorphicStructFactory<T>> factory;

  protected:
    /**
     * Builder for ArionStructType instances.
     * @param[in] name The display name of the structure type.
     * @param[in] factory The shared pointer to the factory defining the structure layout.
     */
    ArionStructType(std::string name, std::shared_ptr<arion_poly_struct::PolymorphicStructFactory<T>> factory)
        : arion_type::KernelType(name, arion::LOG_COLOR::BLUE), factory(factory) {};

  public:
    /**
     * Converts a memory address pointing to the structure in emulated memory to a formatted string representation of
     * the structure content.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] val The memory address pointing to the structure.
     * @return The formatted string representation of the structure content.
     */
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override
    {
        if (!val || !this->arion_is_mapped(arion, val))
            return arion::int_to_hex<uint64_t>(val);
        arion::CPU_ARCH arch = this->arion_curr_arch(arion);
        size_t struct_sz = this->factory->get_struct_sz(arch);
        arion::BYTE *struct_buf = (arion::BYTE *)malloc(struct_sz);
        std::vector<arion::BYTE> struct_data = this->arion_read_mem(arion, val, struct_sz);
        memcpy(struct_buf, struct_data.data(), struct_sz);
        arion_poly_struct::STRUCT_ID struct_id = this->factory->feed(arch, struct_buf);
        std::string struct_str = this->factory->to_string(struct_id, arion, arch);
        free(struct_buf);
        this->factory->release_struct(struct_id);
        return struct_str;
    }
};

/// Represents a variable polymorphic structure type (e.g., `sockaddr`), where the final structure type depends on
/// runtime data (like a family field).
class ArionVariableStructType : public arion_type::KernelType
{
  private:
    /**
     * **(Pure Virtual)** Logic to dynamically determine the concrete structure type based on the content of the
     * emulated memory.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] val The memory address pointing to the generic structure.
     * @return A shared pointer to the concrete `AbsArionStructType` instance (e.g., `StructSockaddrInType`).
     */
    virtual std::shared_ptr<arion_poly_struct::AbsArionStructType> process(std::shared_ptr<arion::Arion> arion,
                                                                           uint64_t val) = 0;

  protected:
    /**
     * Builder for ArionVariableStructType instances.
     * @param[in] name The display name of the structure type.
     */
    ArionVariableStructType(std::string name) : arion_type::KernelType(name, arion::LOG_COLOR::BLUE) {};

  public:
    /**
     * Overrides the string conversion to first call `process` to determine the concrete type, and then uses that
     * concrete type's `str` method.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] val The memory address pointing to the generic structure.
     * @return The formatted string representation of the underlying concrete structure.
     */
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override
    {
        std::shared_ptr<AbsArionStructType> struct_type = this->process(arion, val);
        if (!struct_type)
            return arion_type::INT_TYPE->str(arion, val);
        std::shared_ptr<arion_type::KernelType> type = std::dynamic_pointer_cast<arion_type::KernelType>(struct_type);
        if (!type)
            return arion_type::INT_TYPE->str(arion, val);
        return type->str(arion, val);
    }
};

} // namespace arion_poly_struct

#endif // ARION_STRUCT_UTILS_HPP
