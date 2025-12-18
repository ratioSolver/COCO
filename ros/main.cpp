#include "logging.hpp"
#include <fstream>

int main(int argc, char const *argv[])
{
    std::string module_name;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "-o" && i + 1 < argc)
        {
            module_name = argv[i + 1];
            break;
        }

    LOG_INFO("Generating coco_ros.hpp");
    std::ofstream header_out(module_name + "/coco_ros.hpp", std::ios::out | std::ios::trunc);
    if (!header_out)
    {
        LOG_ERR("Cannot open output file: coco_ros.hpp");
        return 1;
    }
    header_out << "#pragma once\n\n";
    header_out << "#include \"coco_module.hpp\"\n";
    header_out << "#include \"rclcpp/rclcpp.hpp\"\n\n";
    return 0;
}
