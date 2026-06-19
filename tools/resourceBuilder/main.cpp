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

	const std::filesystem::path output_source = output_header;
	std::filesystem::path source_path = output_source;
	source_path.replace_extension(".cpp");

	std::ofstream source(source_path, std::ios::binary);
	if (!source.is_open())
	{
		std::cerr << "Error: could not open output file: " << source_path.string() << "\n";
		return 1;
	}

	output << "#pragma once\n";
	output << "#include <string>\n";
	output << "#include <vector>\n\n";
	output << "namespace spk\n";
	output << "{\n";
	output << "\tconst std::vector<unsigned char>& sparkleResource(const std::string& p_key);\n";
	output << "\tstd::string sparkleResourceAsString(const std::string& p_key);\n";
	output << "\tstd::wstring sparkleResourceAsWString(const std::string& p_key);\n";
	output << "}\n\n";
	output << "#define SPARKLE_GET_RESOURCE(key) spk::sparkleResource(key)\n";
	output << "#define SPARKLE_GET_RESOURCE_AS_STRING(key) spk::sparkleResourceAsString(key)\n";
	output << "#define SPARKLE_GET_RESOURCE_AS_WSTRING(key) spk::sparkleResourceAsWString(key)\n";

	source << "#include \"spk_generated_resources.hpp\"\n\n";
	source << "#include <stdexcept>\n";
	source << "#include <unordered_map>\n\n";
	source << "namespace\n";
	source << "{\n";
	source << "\tconst std::unordered_map<std::string, std::vector<unsigned char>> sparkle_resources = {\n";

	try
	{
		for (std::size_t resource_index = 0; resource_index < input_files.size(); ++resource_index)
		{
			const std::filesystem::path relative_path = input_files[resource_index];
			const std::filesystem::path absolute_path = base_directory / relative_path;
			const std::vector<unsigned char> buffer = read_binary_file(absolute_path);

			source << "\t\t{ \"" << escape_for_cpp_string(relative_path.generic_string()) << "\", {\n\t\t\t";

			for (std::size_t byte_index = 0; byte_index < buffer.size(); ++byte_index)
			{
				source << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[byte_index]) << ", ";

				if ((byte_index + 1) % 16 == 0 && byte_index + 1 < buffer.size())
				{
					source << "\n\t\t\t";
				}
			}

			source << "\n\t\t} }";

			if (resource_index + 1 < input_files.size())
			{
				source << ",";
			}

			source << "\n";
		}
	} catch (const std::exception& exception)
	{
		std::cerr << exception.what() << "\n";
		return 1;
	}

	source << "\t};\n";
	source << "}\n\n";
	source << "namespace spk\n";
	source << "{\n";
	source << "\tconst std::vector<unsigned char>& sparkleResource(const std::string& p_key)\n";
	source << "\t{\n";
	source << "\t\treturn sparkle_resources.at(p_key);\n";
	source << "\t}\n\n";
	source << "\tstd::string sparkleResourceAsString(const std::string& p_key)\n";
	source << "\t{\n";
	source << "\t\tconst auto& resource = sparkleResource(p_key);\n";
	source << "\t\treturn std::string(resource.begin(), resource.end());\n";
	source << "\t}\n\n";
	source << "\tstd::wstring sparkleResourceAsWString(const std::string& p_key)\n";
	source << "\t{\n";
	source << "\t\tconst auto& resource = sparkleResource(p_key);\n";
	source << "\t\treturn std::wstring(resource.begin(), resource.end());\n";
	source << "\t}\n";
	source << "}\n";

	return (output.good() && source.good()) ? 0 : 1;
}
