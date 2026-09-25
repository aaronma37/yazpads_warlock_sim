#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace sim {

class ZipArchive {
public:
    struct ZipEntry {
        std::string filename;
        std::vector<uint8_t> data;
    };

    void add_file(const std::string& filename, const std::string& content) {
        std::vector<uint8_t> bytes(content.begin(), content.end());
        entries.push_back({filename, std::move(bytes)});
    }

    void add_file(const std::string& filename, const std::vector<uint8_t>& data) {
        entries.push_back({filename, data});
    }

    std::vector<uint8_t> build_zip() const {
        std::vector<uint8_t> zip;
        std::vector<uint32_t> local_header_offsets;

        // Write local files
        for (const auto& entry : entries) {
            local_header_offsets.push_back(static_cast<uint32_t>(zip.size()));
            uint32_t crc = calculate_crc32(entry.data.data(), entry.data.size());
            uint32_t size = static_cast<uint32_t>(entry.data.size());
            uint16_t name_len = static_cast<uint16_t>(entry.filename.size());

            // Local file header signature
            append_u32(zip, 0x04034b50);
            append_u16(zip, 20); // Version needed to extract
            append_u16(zip, 0);  // General purpose bit flag
            append_u16(zip, 0);  // Compression method (0 = uncompressed / stored)
            append_u16(zip, 0);  // File last modification time
            append_u16(zip, 0);  // File last modification date
            append_u32(zip, crc);
            append_u32(zip, size); // Compressed size
            append_u32(zip, size); // Uncompressed size
            append_u16(zip, name_len);
            append_u16(zip, 0);  // Extra field length

            // Filename
            zip.insert(zip.end(), entry.filename.begin(), entry.filename.end());
            // File data
            zip.insert(zip.end(), entry.data.begin(), entry.data.end());
        }

        // Write Central Directory
        uint32_t cd_offset = static_cast<uint32_t>(zip.size());
        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& entry = entries[i];
            uint32_t crc = calculate_crc32(entry.data.data(), entry.data.size());
            uint32_t size = static_cast<uint32_t>(entry.data.size());
            uint16_t name_len = static_cast<uint16_t>(entry.filename.size());
            uint32_t offset = local_header_offsets[i];

            // Central directory file header signature
            append_u32(zip, 0x02014b50);
            append_u16(zip, 20); // Version made by
            append_u16(zip, 20); // Version needed to extract
            append_u16(zip, 0);  // General purpose bit flag
            append_u16(zip, 0);  // Compression method (0 = store)
            append_u16(zip, 0);  // File last mod time
            append_u16(zip, 0);  // File last mod date
            append_u32(zip, crc);
            append_u32(zip, size);
            append_u32(zip, size);
            append_u16(zip, name_len);
            append_u16(zip, 0);  // Extra field length
            append_u16(zip, 0);  // File comment length
            append_u16(zip, 0);  // Disk number where file starts
            append_u16(zip, 0);  // Internal file attributes
            append_u32(zip, 0);  // External file attributes
            append_u32(zip, offset); // Relative offset of local file header

            zip.insert(zip.end(), entry.filename.begin(), entry.filename.end());
        }

        uint32_t cd_size = static_cast<uint32_t>(zip.size()) - cd_offset;
        uint16_t total_entries = static_cast<uint16_t>(entries.size());

        // End of central directory record (EOCD)
        append_u32(zip, 0x06054b50);
        append_u16(zip, 0); // Number of this disk
        append_u16(zip, 0); // Disk where central directory starts
        append_u16(zip, total_entries); // Number of central directory records on this disk
        append_u16(zip, total_entries); // Total number of central directory records
        append_u32(zip, cd_size);       // Size of central directory
        append_u32(zip, cd_offset);     // Offset of start of central directory
        append_u16(zip, 0); // ZIP comment length

        return zip;
    }

    bool save_to_file(const std::string& filepath) const {
        auto zip_data = build_zip();
#if defined(__EMSCRIPTEN__)
        EM_ASM({
            var filename = UTF8ToString($0);
            var dataPtr = $1;
            var dataSize = $2;
            var byteArray = new Uint8Array(Module.HEAPU8.buffer, dataPtr, dataSize);
            var blob = new Blob([byteArray], {type: 'application/zip'});
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        }, filepath.c_str(), zip_data.data(), zip_data.size());
        return true;
#else
        std::ofstream out(filepath, std::ios::binary);
        if (!out.is_open()) return false;
        out.write(reinterpret_cast<const char*>(zip_data.data()), zip_data.size());
        return true;
#endif
    }

    static uint32_t calculate_crc32(const uint8_t* data, size_t length) {
        static uint32_t table[256];
        static bool have_table = false;
        if (!have_table) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t rem = i;
                for (int j = 0; j < 8; j++) {
                    if (rem & 1) rem = (rem >> 1) ^ 0xEDB88320;
                    else rem >>= 1;
                }
                table[i] = rem;
            }
            have_table = true;
        }
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; i++) {
            crc = (crc >> 8) ^ table[(crc & 0xFF) ^ data[i]];
        }
        return crc ^ 0xFFFFFFFF;
    }

private:
    std::vector<ZipEntry> entries;

    static void append_u16(std::vector<uint8_t>& buf, uint16_t val) {
        buf.push_back(static_cast<uint8_t>(val & 0xFF));
        buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    }

    static void append_u32(std::vector<uint8_t>& buf, uint32_t val) {
        buf.push_back(static_cast<uint8_t>(val & 0xFF));
        buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    }
};

} // namespace sim
