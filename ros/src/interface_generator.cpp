#include "interface_generator.hpp"
#include "logging.hpp"
#include <fstream>

namespace coco
{
    interface_generator::interface_generator(std::vector<std::filesystem::path> &&type_files, std::filesystem::path &&output_dir) : output_dir(std::move(output_dir))
    {
        for (const auto &file : type_files)
        {
            std::ifstream in(file);
            if (!in)
                throw std::runtime_error("Cannot open type file: " + file.string());
            auto j = json::load(in);
            types[j["name"].get<std::string>()] = std::move(j);
        }
    }

    void interface_generator::generate_messages()
    {
        auto msg_dir = output_dir / "msg";
        if (!std::filesystem::exists(msg_dir))
        {
            LOG_INFO("Creating message directory: " << msg_dir.string());
            std::filesystem::create_directories(msg_dir);
        }
        for (const auto &[name, j_tp] : types)
        {
            auto ros_name = to_ros_identifier(name);
            std::ofstream msg_file((msg_dir / (ros_name + ".msg")).string(), std::ios::out | std::ios::trunc);
            if (!msg_file)
                throw std::runtime_error("Cannot create message file: " + ros_name + ".msg");
            LOG_INFO("Generating message file for type: " << name);
            if (j_tp.contains("dynamic_properties"))
                for (const auto &[prop_name, prop_value] : j_tp["dynamic_properties"].as_object())
                    msg_file << prop_to_ros(prop_name, prop_value);
        }
    }

    void interface_generator::generate_package_xml()
    {
        std::ofstream pkg_file(output_dir / "package.xml", std::ios::out | std::ios::trunc);
        if (!pkg_file)
            throw std::runtime_error("Cannot open output file: package.xml");

        pkg_file << "<?xml version=\"1.0\"?>\n";
        pkg_file << "<package format=\"2\">\n";
        pkg_file << "  <name>coco_ros_interfaces</name>\n";
        pkg_file << "  <version>0.1.0</version>\n";
        pkg_file << "  <description>COCO ROS Interfaces</description>\n";
        pkg_file << "  <maintainer email=\"riccardo.debenedictis@cnr.it\">Riccardo De Benedictis</maintainer>\n";
        pkg_file << "  <license>Apache-2.0</license>\n";
        pkg_file << "  <buildtool_depend>ament_cmake</buildtool_depend>\n";
        pkg_file << "  <build_depend>rosidl_default_generators</build_depend>\n";
        pkg_file << "  <exec_depend>rosidl_default_runtime</exec_depend>\n";
        pkg_file << "</package>\n";
    }

    void interface_generator::generate_cmake_lists()
    {
        std::ofstream cmake_out(output_dir / "CMakeLists.txt", std::ios::out | std::ios::trunc);
        if (!cmake_out)
            throw std::runtime_error("Cannot open output file: CMakeLists.txt");

        cmake_out << "cmake_minimum_required(VERSION 3.5)\n";
        cmake_out << "project(coco_ros_interfaces)\n\n";
        cmake_out << "find_package(ament_cmake REQUIRED)\n";
        cmake_out << "find_package(rosidl_default_generators REQUIRED)\n\n";
        cmake_out << "rosidl_generate_interfaces(${PROJECT_NAME}\n";
        for (const auto &[name, j_tp] : types)
        {
            auto ros_name = to_ros_identifier(name);
            cmake_out << "  \"msg/" << ros_name << ".msg\"\n";
        }
        cmake_out << ")\n\n";
        cmake_out << "ament_export_dependencies(rosidl_default_runtime)\n\n";
        cmake_out << "ament_package()\n";
    }

    std::string interface_generator::to_ros_identifier(const std::string &symbol) noexcept
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

    std::string interface_generator::prop_to_ros(const std::string &name, const json::json &prop)
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
} // namespace coco
