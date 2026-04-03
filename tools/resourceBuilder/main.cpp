#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::string escape_for_cpp_string(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());

        for (const char character : value)
        {
            switch (character)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += character;
                break;
            }
        }

        return escaped;
    }

    std::vector<unsigned char> read_binary_file(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input.is_open())
        {
            throw std::runtime_error("Error: cannot open " + path.string());
        }

        const std::streamsize file_size = input.tellg();
        if (file_size < 0)
        {
            throw std::runtime_error("Error: failed to read size of " + path.string());
        }

        input.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(static_cast<std::size_t>(file_size));
        if (file_size > 0 && !input.read(reinterpret_cast<char*>(buffer.data()), file_size))
        {
            throw std::runtime_error("Error: failed to read " + path.string());
        }

        return buffer;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: sparkleResourcesConverter <outputHeader.hpp> <baseDirectory> <file1> [file2] ...\n";
        return 1;
    }

    const std::filesystem::path output_header = argv[1];
    const std::filesystem::path base_directory = argv[2];

    std::vector<std::string> input_files;
    input_files.reserve(static_cast<std::size_t>(argc - 3));

    for (int index = 3; index < argc; ++index)
    {
        input_files.emplace_back(argv[index]);
    }

    if (output_header.has_parent_path())
    {
        std::filesystem::create_directories(output_header.parent_path());
    }

    std::ofstream output(output_header, std::ios::binary);
    if (!output.is_open())
    {
        std::cerr << "Error: could not open output file: " << output_header.string() << "\n";
        return 1;
    }

    output << "#pragma once\n";
    output << "#include <string>\n";
    output << "#include <unordered_map>\n";
    output << "#include <vector>\n\n";
    output << "inline const std::unordered_map<std::string, std::vector<unsigned char>> sparkle_resources = {\n";

    try
    {
        for (std::size_t resource_index = 0; resource_index < input_files.size(); ++resource_index)
        {
            const std::filesystem::path relative_path = input_files[resource_index];
            const std::filesystem::path absolute_path = base_directory / relative_path;
            const std::vector<unsigned char> buffer = read_binary_file(absolute_path);

            output << "    { \"" << escape_for_cpp_string(relative_path.generic_string()) << "\", {\n        ";

            for (std::size_t byte_index = 0; byte_index < buffer.size(); ++byte_index)
            {
                output << "0x" << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<int>(buffer[byte_index]) << ", ";

                if ((byte_index + 1) % 16 == 0 && byte_index + 1 < buffer.size())
                {
                    output << "\n        ";
                }
            }

            output << "\n    } }";

            if (resource_index + 1 < input_files.size())
            {
                output << ",";
            }

            output << "\n";
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << "\n";
        return 1;
    }

    output << "};\n\n";
    output << "#define SPARKLE_GET_RESOURCE(key) sparkle_resources.at(key)\n";
    output << "#define SPARKLE_GET_RESOURCE_AS_STRING(key) std::string(sparkle_resources.at(key).begin(), sparkle_resources.at(key).end())\n";
    output << "#define SPARKLE_GET_RESOURCE_AS_WSTRING(key) std::wstring(sparkle_resources.at(key).begin(), sparkle_resources.at(key).end())\n";

    return output.good() ? 0 : 1;
}
