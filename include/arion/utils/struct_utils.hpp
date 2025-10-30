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
using STRUCT_ID = uint64_t;

inline std::map<arion::CPU_ARCH, uint8_t> ALIGN_BY_ARCH{
    {arion::CPU_ARCH::X86_ARCH, 4},
    {arion::CPU_ARCH::X8664_ARCH, 8},
    {arion::CPU_ARCH::ARM_ARCH, 8},
    {arion::CPU_ARCH::ARM64_ARCH, 8},
};

inline std::map<arion::CPU_ARCH, uint8_t> PTR_SZ_BY_ARCH{
    {arion::CPU_ARCH::X86_ARCH, 4},
    {arion::CPU_ARCH::X8664_ARCH, 8},
    {arion::CPU_ARCH::ARM_ARCH, 4},
    {arion::CPU_ARCH::ARM64_ARCH, 8},
};

inline std::map<arion::CPU_ARCH, uint16_t> ARCH_SZ{
    {arion::CPU_ARCH::X86_ARCH, 32},
    {arion::CPU_ARCH::X8664_ARCH, 64},
    {arion::CPU_ARCH::ARM_ARCH, 32},
    {arion::CPU_ARCH::ARM64_ARCH, 64},
};

enum POLYMORPHIC_STRUCT_FIELD_TYPE
{
    V8,
    V16,
    V32,
    V64,
    PTR_SZ,
    A8,
    A16,
    A32,
    A64,
    PTR_ARR
};

struct POLYMORPHIC_STRUCT_CONSTRAINT
{
    arion::CPU_ARCH arch;
    uint16_t arch_sz;

    POLYMORPHIC_STRUCT_CONSTRAINT() : arch(arion::CPU_ARCH::UNKNOWN_ARCH), arch_sz(0) {};
    POLYMORPHIC_STRUCT_CONSTRAINT(arion::CPU_ARCH arch) : arch(arch), arch_sz(0) {};
    POLYMORPHIC_STRUCT_CONSTRAINT(uint16_t arch_sz) : arch(arion::CPU_ARCH::UNKNOWN_ARCH), arch_sz(arch_sz) {};
};

struct POLYMORPHIC_STRUCT_FIELD
{
    POLYMORPHIC_STRUCT_CONSTRAINT constraint;
    POLYMORPHIC_STRUCT_FIELD_TYPE type;
    std::shared_ptr<arion_type::KernelType> arion_type;
    std::string name;
    size_t sz;

    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_CONSTRAINT constraint, POLYMORPHIC_STRUCT_FIELD_TYPE type,
                             std::shared_ptr<arion_type::KernelType> arion_type, std::string name, size_t sz)
        : constraint(constraint), type(type), arion_type(arion_type), name(name), sz(sz) {};
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_CONSTRAINT constraint, POLYMORPHIC_STRUCT_FIELD_TYPE type,
                             std::shared_ptr<arion_type::KernelType> arion_type, std::string name)
        : POLYMORPHIC_STRUCT_FIELD(constraint, type, arion_type, name, 0) {};
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_FIELD_TYPE type, std::shared_ptr<arion_type::KernelType> arion_type,
                             std::string name, size_t sz)
        : POLYMORPHIC_STRUCT_FIELD({}, type, arion_type, name, sz) {};
    POLYMORPHIC_STRUCT_FIELD(POLYMORPHIC_STRUCT_FIELD_TYPE type, std::shared_ptr<arion_type::KernelType> arion_type,
                             std::string name)
        : POLYMORPHIC_STRUCT_FIELD({}, type, arion_type, name, 0) {};
};

class PolymorphicStruct
{
  private:
    std::map<std::string, uint64_t> int_vals;
    std::map<std::string, void *> arr_vals;

  public:
    ~PolymorphicStruct()
    {
        for (auto arr_it : this->arr_vals)
            free(arr_it.second);
    }
    void set_field(std::string name, uint64_t val)
    {
        this->int_vals[name] = val;
    }
    void set_field(std::string name, void *arr)
    {
        auto arr_it = this->arr_vals.find(name);
        if (arr_it != this->arr_vals.end())
            free(arr_it->second);
        this->arr_vals[name] = arr;
    }
    bool has_int_field(std::string name)
    {
        auto int_it = this->int_vals.find(name);
        return int_it != this->int_vals.end();
    }
    uint64_t get_int_field(std::string name)
    {
        auto int_it = this->int_vals.find(name);
        if (int_it == this->int_vals.end())
            throw arion_exception::NoStructFieldWithNameException(name);
        return int_it->second;
    }
    bool has_arr_field(std::string name)
    {
        auto arr_it = this->arr_vals.find(name);
        return arr_it != this->arr_vals.end();
    }
    void *get_arr_field(std::string name)
    {
        auto arr_it = this->arr_vals.find(name);
        if (arr_it == this->arr_vals.end())
            throw arion_exception::NoStructFieldWithNameException(name);
        return arr_it->second;
    }
};

template <typename T> class PolymorphicStructFactory
{
  private:
    STRUCT_ID curr_id = 1;
    std::stack<STRUCT_ID> free_ids;
    std::map<STRUCT_ID, std::unique_ptr<PolymorphicStruct>> curr_structs;
    std::vector<POLYMORPHIC_STRUCT_FIELD> fields;

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

    STRUCT_ID add_struct_entry(std::unique_ptr<PolymorphicStruct> struct_inst)
    {
        STRUCT_ID id = this->gen_next_id();
        this->curr_structs[id] = std::move(struct_inst);
        return id;
    }

    uint8_t get_align(arion::CPU_ARCH arch)
    {
        auto align_it = ALIGN_BY_ARCH.find(arch);
        if (align_it == ALIGN_BY_ARCH.end())
            throw arion_exception::UnsupportedCpuArchException();
        return align_it->second;
    }

    uint8_t get_ptr_sz(arion::CPU_ARCH arch)
    {
        auto ptr_sz_it = PTR_SZ_BY_ARCH.find(arch);
        if (ptr_sz_it == PTR_SZ_BY_ARCH.end())
            throw arion_exception::UnsupportedCpuArchException();
        return ptr_sz_it->second;
    }

    uint16_t get_arch_sz(arion::CPU_ARCH arch)
    {
        auto arch_sz_it = ARCH_SZ.find(arch);
        if (arch_sz_it == ARCH_SZ.end())
            throw arion_exception::UnsupportedCpuArchException();
        return arch_sz_it->second;
    }

  protected:
    PolymorphicStructFactory(std::vector<POLYMORPHIC_STRUCT_FIELD> fields) : fields(std::move(fields)) {};
    ~PolymorphicStructFactory()
    {
        this->clear_structs();
    }

    // Macros for syntax simplification in subclasses.
    using ftype = arion_poly_struct::POLYMORPHIC_STRUCT_FIELD_TYPE;

  public:
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

    size_t get_host_struct_sz()
    {
        return this->get_struct_sz(arion::get_host_cpu_arch());
    }

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

    STRUCT_ID feed_host(T *struct_inst)
    {
        arion::BYTE *data = (arion::BYTE *)malloc(sizeof(T));
        memcpy(data, struct_inst, sizeof(T));
        STRUCT_ID id = this->feed(arion::get_host_cpu_arch(), data);
        free(data);
        return id;
    }

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
                        memcpy(&val, (void *)((uint64_t)arr + arr_i * arch_sz), arch_sz);
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

        return ss.str();
    }

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

    void clear_structs()
    {
        std::vector<STRUCT_ID> struct_ids; // need to clone keys to prevent concurrency editing
        for (const auto &struct_pair : this->curr_structs)
            struct_ids.push_back(struct_pair.first);
        for (STRUCT_ID struct_id : struct_ids)
            this->release_struct(struct_id);
    }
};

class AbsArionStructType
{
  protected:
    virtual ~AbsArionStructType() = default;
    bool arion_is_mapped(std::shared_ptr<arion::Arion> arion, arion::ADDR addr);
    arion::CPU_ARCH arion_curr_arch(std::shared_ptr<arion::Arion> arion);
    std::vector<arion::BYTE> arion_read_mem(std::shared_ptr<arion::Arion> arion, arion::ADDR addr, size_t sz);
};

template <typename T> class ArionStructType : public arion_type::KernelType, public AbsArionStructType
{
  private:
    std::shared_ptr<arion_poly_struct::PolymorphicStructFactory<T>> factory;

  protected:
    ArionStructType(std::string name, std::shared_ptr<arion_poly_struct::PolymorphicStructFactory<T>> factory)
        : arion_type::KernelType(name, arion::LOG_COLOR::BLUE), factory(factory) {};

  public:
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

class ArionVariableStructType : public arion_type::KernelType
{
  private:
    virtual std::shared_ptr<arion_poly_struct::AbsArionStructType> process(std::shared_ptr<arion::Arion> arion,
                                                                           uint64_t val) = 0;

  protected:
    ArionVariableStructType(std::string name) : arion_type::KernelType(name, arion::LOG_COLOR::BLUE) {};

  public:
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
