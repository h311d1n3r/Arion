#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/crypto/md5.hpp>
#include <arion/utils/fs_utils.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <uuid/uuid.h>

using namespace arion;

void read_bin_file(std::string file_path, ADDR off, size_t sz, RD_BIN_CALLBACK callback)
{
    if (!std::filesystem::exists(file_path))
        throw FileNotFoundException(file_path);
    std::ifstream rd_file(file_path, std::ios::binary);
    if (!rd_file)
        throw FileOpenException(file_path);
    rd_file.seekg(off);
    std::array<BYTE, ARION_BUF_SZ> buf;
    for (ADDR curr_off = 0; curr_off < sz; curr_off += ARION_BUF_SZ)
    {
        size_t read_sz = ARION_BUF_SZ;
        size_t remaining_sz = sz - curr_off;
        if (remaining_sz < ARION_BUF_SZ)
            read_sz = remaining_sz;
        rd_file.read((char *)buf.data(), read_sz);
        callback(buf, curr_off, read_sz);
    }
    rd_file.close();
}

std::string gen_tmp_path()
{
    const std::string tmp_dir = "/tmp/";
    if (!std::filesystem::exists(tmp_dir))
        throw FileNotFoundException(tmp_dir);

    std::string file_path;
    std::string file_name;
    uuid_t uuid;
    char uuid_str[ARION_UUID_SZ + 1];
    do
    {
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);

        file_name = std::string(uuid_str) + ".tmp";
        file_path = tmp_dir + file_name;
    } while (std::filesystem::exists(file_path));

    return file_path;
}

std::string md5_hash_file(std::string file_path)
{
    FILE *in_f = fopen(file_path.c_str(), "rb");
    if (!in_f)
        throw FileOpenException(file_path);

    uint8_t *digest = (uint8_t *)malloc(MD5_DIGEST_LEN);
    md5_file(in_f, digest);

    std::stringstream hash_ss;
    for (size_t hash_i = 0; hash_i < MD5_DIGEST_LEN; hash_i++)
    {
        hash_ss << std::hex << std::setw(2) << std::setfill('0') << +digest[hash_i];
    }

    free(digest);
    fclose(in_f);

    return hash_ss.str();
}
