#include "config_generator.hpp"
#include "json.hpp"
#include "logging.hpp"
#include <filesystem>
#include <regex>

namespace coco
{
    void config_generator::generate_config(const std::vector<std::string> &type_files, const std::vector<std::string> &rule_files, const std::string &output_file)
    {
        LOG_DEBUG("Generating config to " << output_file);

        try
        {
            std::filesystem::path outp(output_file);
            auto parent = outp.parent_path();
            if (!parent.empty())
            {
                std::error_code ec;
                std::filesystem::create_directories(parent, ec);
                if (ec)
                {
                    LOG_ERR("Cannot create directories for output: " << parent << " : " << ec.message());
                    return;
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERR("Filesystem error while preparing output path: " << e.what());
            return;
        }

        std::ofstream out(output_file, std::ios::out | std::ios::trunc);
        if (!out)
        {
            LOG_ERR("Cannot open output file: " << output_file);
            return;
        }

        out << "// This file is auto-generated. Do not edit manually.\n\n";
        out << "#pragma once\n\n";
        out << "#include \"coco.hpp\"\n";
        out << "#include \"logging.hpp\"\n";
        out << "#include <fstream>\n\n";

        out << "[[nodiscard]] inline std::string read_rule(const std::string &path)\n{\n";
        out << "    std::ifstream in(path);\n";
        out << "    if (!in)\n";
        out << "        throw std::runtime_error(\"Cannot open rule file: \" + path);\n";
        out << "    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());\n";
        out << "}\n\n";

        out << "inline void load_config(coco::coco &cc)\n{\n";
        generate_types(type_files, out);
        generate_rules(rule_files, out);
        out << "}\n";
    }

    void config_generator::generate_types(const std::vector<std::string> &type_files, std::ofstream &out)
    {
        for (const auto &tp_file : type_files)
        {
            LOG_DEBUG("Loading " << tp_file);
            std::ifstream in(tp_file);
            if (!in)
                throw std::runtime_error("Cannot open type file: " + tp_file);
            json::json j_t = json::load(in);
            LOG_DEBUG("Loaded type: " << j_t.dump());

            std::string tp_name = j_t["name"].get<std::string>();
            std::string static_props = j_t.contains("static_properties") ? "json::load(R\"(" + j_t["static_properties"].dump() + ")\")" : "{}";
            std::string dynamic_props = j_t.contains("dynamic_properties") ? "json::load(R\"(" + j_t["dynamic_properties"].dump() + ")\")" : "{}";
            std::string data = j_t.contains("data") ? "json::load(R\"(" + j_t["data"].dump() + ")\")" : "{}";
            std::string cpp_tp_name = to_cpp_identifier(tp_name);
            out << "    try {\n";
            out << "        [[maybe_unused]] auto &" << cpp_tp_name << " = cc.get_type(\"" << tp_name << "\");\n";
            out << "        LOG_DEBUG(\"Type `" << tp_name << "` found\");\n";
            out << "    } catch (const std::invalid_argument &e) {\n";
            out << "        LOG_DEBUG(\"Creating `" << tp_name << "` type\");\n";
            out << "        [[maybe_unused]] auto &" << cpp_tp_name << " = cc.create_type(\"" << tp_name << "\", " << static_props << ", " << dynamic_props << ", " << data << ");\n";
            out << "    }\n";
        }
    }

    void config_generator::generate_rules(const std::vector<std::string> &rule_files, std::ofstream &out)
    {
        out << "    cc.load_rules();\n";

        for (const auto &r_file : rule_files)
        {
            LOG_DEBUG("Loading " << r_file);
            std::ifstream in(r_file);
            if (!in)
                throw std::runtime_error("Cannot open rule file: " + r_file);
            std::filesystem::path rp_path(r_file);
            std::string name_no_ext = rp_path.stem().string();
            out << "    try {\n";
            out << "        [[maybe_unused]] auto &" << name_no_ext << "_rule = cc.get_reactive_rule(\"" << name_no_ext << "\");\n";
            out << "        LOG_DEBUG(\"Reactive rule `" << name_no_ext << "` found\");\n";
            out << "    } catch (const std::invalid_argument &e) {\n";
            out << "        LOG_DEBUG(\"Creating `" << name_no_ext << "` reactive rule\");\n";
            out << "        [[maybe_unused]] auto &" << name_no_ext << "_rule = cc.create_reactive_rule(\"" << name_no_ext << "\", read_rule(\"" << r_file << "\"));\n";
            out << "    }\n";
        }
    }

    std::string config_generator::to_cpp_identifier(const std::string &symbol)
    {
        std::string result;
        for (char c : symbol)
            if (std::isalnum(c) || c == '_')
                result += c;
            else
                result += '_';
        if (!result.empty() && std::isdigit(result[0]))
            result = "_" + result;
        result = std::regex_replace(result, std::regex("_+"), "_");
        return result;
    }
} // namespace coco
