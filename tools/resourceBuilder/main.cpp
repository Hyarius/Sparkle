#include <filesystem>
#include <fstream>
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
	source << "#include <filesystem>\n";
	source << "#include <fstream>\n";
	source << "#include <mutex>\n";
	source << "#include <stdexcept>\n";
	source << "#include <unordered_map>\n\n";
	source << "namespace\n";
	source << "{\n";
	source << "\tconst std::unordered_map<std::string, std::filesystem::path> sparkle_resource_paths = {\n";

	try
	{
		for (std::size_t resource_index = 0; resource_index < input_files.size(); ++resource_index)
		{
			const std::filesystem::path relative_path = input_files[resource_index];
			const std::filesystem::path absolute_path = base_directory / relative_path;

			// Validate the resource while generating the access table so missing files
			// fail at configure/build time rather than later during a test run.
			(void)read_binary_file(absolute_path);

			source << "\t\t{ \"" << escape_for_cpp_string(relative_path.generic_string()) << "\", \""
				   << escape_for_cpp_string(absolute_path.lexically_normal().generic_string()) << "\" }";

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

	source << "\t};\n\n";
	source << "\tstd::unordered_map<std::string, std::vector<unsigned char>> sparkle_resource_cache;\n";
	source << "\tstd::mutex sparkle_resource_cache_mutex;\n\n";
	source << "\tstd::vector<unsigned char> loadResource(const std::filesystem::path& p_path)\n";
	source << "\t{\n";
	source << "\t\tstd::ifstream input(p_path, std::ios::binary | std::ios::ate);\n";
	source << "\t\tif (!input.is_open())\n";
	source << "\t\t{\n";
	source << "\t\t\tthrow std::runtime_error(\"Error: cannot open resource \" + p_path.string());\n";
	source << "\t\t}\n\n";
	source << "\t\tconst std::streamsize fileSize = input.tellg();\n";
	source << "\t\tif (fileSize < 0)\n";
	source << "\t\t{\n";
	source << "\t\t\tthrow std::runtime_error(\"Error: failed to read resource size \" + p_path.string());\n";
	source << "\t\t}\n\n";
	source << "\t\tinput.seekg(0, std::ios::beg);\n";
	source << "\t\tstd::vector<unsigned char> buffer(static_cast<std::size_t>(fileSize));\n";
	source << "\t\tif (fileSize > 0 && !input.read(reinterpret_cast<char*>(buffer.data()), fileSize))\n";
	source << "\t\t{\n";
	source << "\t\t\tthrow std::runtime_error(\"Error: failed to read resource \" + p_path.string());\n";
	source << "\t\t}\n\n";
	source << "\t\treturn buffer;\n";
	source << "\t}\n";
	source << "}\n\n";
	source << "namespace spk\n";
	source << "{\n";
	source << "\tconst std::vector<unsigned char>& sparkleResource(const std::string& p_key)\n";
	source << "\t{\n";
	source << "\t\tstd::lock_guard<std::mutex> lock(sparkle_resource_cache_mutex);\n";
	source << "\t\tauto cacheIt = sparkle_resource_cache.find(p_key);\n";
	source << "\t\tif (cacheIt != sparkle_resource_cache.end())\n";
	source << "\t\t{\n";
	source << "\t\t\treturn cacheIt->second;\n";
	source << "\t\t}\n\n";
	source << "\t\tconst auto pathIt = sparkle_resource_paths.find(p_key);\n";
	source << "\t\tif (pathIt == sparkle_resource_paths.end())\n";
	source << "\t\t{\n";
	source << "\t\t\tthrow std::out_of_range(\"Unknown Sparkle resource: \" + p_key);\n";
	source << "\t\t}\n\n";
	source << "\t\tauto [insertedIt, inserted] = sparkle_resource_cache.emplace(p_key, loadResource(pathIt->second));\n";
	source << "\t\treturn insertedIt->second;\n";
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
