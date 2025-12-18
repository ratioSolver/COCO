#include "interface_generator.hpp"
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>

int main(int argc, char const *argv[])
{
    std::vector<std::filesystem::path> type_files;
    std::filesystem::path module_name;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "-t" && i + 1 < argc)
        {
            while (i + 1 < argc && std::string(argv[i + 1])[0] != '-')
            {
                type_files.push_back(argv[i + 1]);
                ++i;
            }
            continue;
        }
        else if (std::string(argv[i]) == "-tf" && i + 1 < argc)
        {
            while (i + 1 < argc && std::string(argv[i + 1])[0] != '-')
            {
                for (const auto &entry : std::filesystem::directory_iterator(argv[i + 1]))
                    if (entry.is_regular_file())
                        type_files.push_back(entry.path().string());
                ++i;
            }
            continue;
        }
        else if (std::string(argv[i]) == "-o" && i + 1 < argc)
        {
            module_name = argv[i + 1];
            break;
        }

    coco::interface_generator generator(std::move(type_files), std::filesystem::path(module_name));
    generator.generate_messages();
    generator.generate_package_xml();
    generator.generate_cmake_lists();

    return 0;
}
