#include "json.hpp"
#include "logging.hpp"
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>

std::string to_ros_identifier(const std::string &symbol)
{
    std::string result;
    bool cap_next = false;
    for (char c : symbol)
        if (c == '_' || c == ' ' || c == '-')
            cap_next = true;
        else if (cap_next)
        {
            result += std::toupper(c);
            cap_next = false;
        }
        else
            result += c;
    if (result.empty() || std::isdigit(result[0]))
        result.insert(result.begin(), 'T');
    else
        result.front() = std::toupper(result.front());
    return result;
}

std::string prop_to_ros(const std::string &name, const json::json &prop)
{
    std::string prop_tp = prop["type"].get<std::string>();
    std::string ros_def;
    bool multiple = prop.contains("multiple") && prop["multiple"].get<bool>();
    if (prop_tp == "int")
        ros_def = "int32";
    else if (prop_tp == "float")
        ros_def = "float32";
    else if (prop_tp == "string")
        ros_def = "string";
    else if (prop_tp == "symbol")
        ros_def = "string";
    else if (prop_tp == "bool")
        ros_def = "bool";
    else if (prop_tp == "item")
        ros_def = "string";
    else
        throw std::runtime_error("Unsupported property type for ROS message: " + prop_tp);

    if (multiple)
        ros_def += "[]";

    return ros_def + " " + name + "\n";
}

int main(int argc, char const *argv[])
{
    std::vector<std::string> type_files;
    std::string module_name;
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

    std::unordered_map<std::string, json::json> types;
    LOG_INFO("Loading " << type_files.size() << " type files");
    for (const auto &tp_file : type_files)
    {
        LOG_INFO("Loading " << tp_file);
        std::ifstream in(tp_file);
        if (!in)
        {
            LOG_ERR("Cannot open type file: " << tp_file);
            return 1;
        }
        json::json j_t = json::load(in);
        auto tp_name = j_t["name"].get<std::string>();
        types[tp_name] = j_t;
        LOG_INFO("Loaded type: " << tp_name);
    }

    LOG_INFO("Creating message directory");
    auto msg_dir = std::filesystem::path(module_name) / "msg";
    if (!std::filesystem::exists(msg_dir))
        if (!std::filesystem::create_directories(msg_dir))
        {
            LOG_ERR("Failed to create 'msg' directory");
            return 1;
        }
    LOG_INFO("Generating message files");
    for (const auto &[name, j_tp] : types)
    {
        auto ros_name = to_ros_identifier(name);
        std::ofstream msg_file((msg_dir / (ros_name + ".msg")).string(), std::ios::out | std::ios::trunc);
        if (!msg_file)
        {
            LOG_ERR("Cannot create message file: " << ros_name << ".msg");
            return 1;
        }
        LOG_INFO("Generating message file for type: " << name);
        if (j_tp.contains("dynamic_properties"))
            for (const auto &[prop_name, prop_value] : j_tp["dynamic_properties"].as_object())
                msg_file << prop_to_ros(prop_name, prop_value);
    }

    LOG_INFO("Generating package.xml");
    std::ofstream pkg_file(module_name + "/package.xml", std::ios::out | std::ios::trunc);
    if (!pkg_file)
    {
        LOG_ERR("Cannot open output file: package.xml");
        return 1;
    }
    pkg_file << "<?xml version=\"1.0\"?>\n";
    pkg_file << "<package format=\"2\">\n";
    pkg_file << "  <name>ros_coco</name>\n";
    pkg_file << "  <version>0.1.0</version>\n";
    pkg_file << "  <description>COCO ROS Module</description>\n";
    pkg_file << "  <maintainer email=\"riccardo.debenedictis@cnr.it\">Riccardo De Benedictis</maintainer>\n";
    pkg_file << "  <license>MIT</license>\n";
    pkg_file << "  <buildtool_depend>ament_cmake</buildtool_depend>\n";
    pkg_file << "  <build_depend>rclcpp</build_depend>\n";
    pkg_file << "  <exec_depend>rclcpp</exec_depend>\n";
    pkg_file << "  <build_depend>rosidl_default_generators</build_depend>\n";
    pkg_file << "  <exec_depend>rosidl_default_runtime</exec_depend>\n";
    pkg_file << "</package>\n";

    LOG_INFO("Generating coco_ros.hpp");
    std::ofstream header_out(module_name + "/coco_ros.hpp", std::ios::out | std::ios::trunc);
    if (!header_out)
    {
        LOG_ERR("Cannot open output file: coco_ros.hpp");
        return 1;
    }
    header_out << "#pragma once\n\n";
    header_out << "#include \"coco_module.hpp\"\n";
    header_out << "#include \"coco.hpp\"\n";
    header_out << "#include \"rclcpp/rclcpp.hpp\"\n\n";
    header_out << "namespace coco\n{\n";
    header_out << "  class coco_ros : public coco_module, private listener\n";
    header_out << "  {\n";
    header_out << "  public:\n";
    header_out << "    coco_ros(coco &cc) noexcept;\n";
    header_out << "  };\n";
    header_out << "} // namespace coco\n\n";

    LOG_INFO("Generating coco_ros.cpp");
    std::ofstream source_out(module_name + "/coco_ros.cpp", std::ios::out | std::ios::trunc);
    if (!source_out)
    {
        LOG_ERR("Cannot open output file: coco_ros.cpp");
        return 1;
    }
    source_out << "#include \"coco_ros.hpp\"\n\n";
    source_out << "namespace coco\n{\n";
    source_out << "    coco_ros::coco_ros(coco &cc) noexcept : coco_module(cc), listener(cc)\n";
    source_out << "    {}\n";
    source_out << "} // namespace coco\n";
    return 0;
}
