#include "config_generator.hpp"
#include "json.hpp"
#include "logging.hpp"
#include <filesystem>
#include <regex>

namespace coco
{
    config_generator::config_generator(const std::vector<std::string> &type_files, const std::vector<std::string> &rule_files, const std::string &output_file) : output_file(output_file)
    {
        LOG_DEBUG("Loading types");
        for (const auto &tp_file : type_files)
        {
            LOG_DEBUG("Loading " << tp_file);
            std::ifstream in(tp_file);
            if (!in)
                throw std::runtime_error("Cannot open type file: " + tp_file);
            json::json j_t = json::load(in);
            auto tp_name = j_t["name"].get<std::string>();
            types[tp_name] = j_t;
            LOG_DEBUG("Loaded type: " << tp_name);
        }

        LOG_DEBUG("Loading rules");
        for (const auto &r_file : rule_files)
        {
            LOG_DEBUG("Loading " << r_file);
            std::ifstream in(r_file);
            if (!in)
                throw std::runtime_error("Cannot open rule file: " + r_file);
            std::filesystem::path rp_path(r_file);
            std::string name_no_ext = rp_path.stem().string();
            rules[name_no_ext] = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        }
    }

    void config_generator::generate_types(std::ofstream &out)
    {
        out << "    std::unordered_map<std::string, json::json> types;\n";
        for (const auto &[tp_name, j_t] : types)
            out << "    types[\"" << tp_name << "\"] = json::load(R\"(" << j_t.dump() << ")\");\n";

        out << "    std::vector<std::string> created_types;\n";
        for (const auto &[tp_name, j_t] : types)
        {
            // std::string static_props = j_t.contains("static_properties") ? "json::load(R\"(" + j_t["static_properties"].dump() + ")\")" : "{}";
            // std::string dynamic_props = j_t.contains("dynamic_properties") ? "json::load(R\"(" + j_t["dynamic_properties"].dump() + ")\")" : "{}";
            // std::string data = j_t.contains("data") ? "json::load(R\"(" + j_t["data"].dump() + ")\")" : "{}";
            std::string cpp_tp_name = to_cpp_identifier(tp_name);
            out << "    try {\n";
            out << "        [[maybe_unused]] auto &" << cpp_tp_name << " = cc.get_type(\"" << tp_name << "\");\n";
            out << "        LOG_DEBUG(\"Type `" << tp_name << "` found\");\n";
            out << "    } catch (const std::invalid_argument &e) {\n";
            out << "        LOG_DEBUG(\"Creating `" << tp_name << "` type\");\n";
            out << "        [[maybe_unused]] auto &" << cpp_tp_name << " = cc.make_type(\"" << tp_name << "\", types.at(\"" << tp_name << "\").contains(\"data\") ? types.at(\"" << tp_name << "\")[\"data\"] : {});\n";
            out << "        created_types.push_back(\"" << tp_name << "\");\n";
            out << "    }\n";
        }
        out << "    for (const auto &tp_name : created_types)";
        out << "        cc.get_type(tp_name).set_properties(types.at(tp_name).contains(\"static_properties\") ? types.at(tp_name)[\"static_properties\"] : {}, types.at(tp_name).contains(\"dynamic_properties\") ? types.at(tp_name)[\"dynamic_properties\"] : {});\n";
    }

    void config_generator::generate_rules(std::ofstream &out)
    {
        out << "    cc.load_rules();\n";
        for (const auto &[r_name, r_content] : rules)
        {
            out << "    try {\n";
            out << "        [[maybe_unused]] auto &" << r_name << "_rule = cc.get_reactive_rule(\"" << r_name << "\");\n";
            out << "        LOG_DEBUG(\"Reactive rule `" << r_name << "` found\");\n";
            out << "    } catch (const std::invalid_argument &e) {\n";
            out << "        LOG_DEBUG(\"Creating `" << r_name << "` reactive rule\");\n";
            out << "        [[maybe_unused]] auto &" << r_name << "_rule = cc.create_reactive_rule(\"" << r_name << "\", \"" << r_content << "\");\n";
            out << "    }\n";
        }
    }

    void config_generator::generate_messages(const std::vector<std::string> &type_files)
    {
        for (const auto &tp_file : type_files)
        {
        }
    }

    void config_generator::generate_config()
    {
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

        LOG_DEBUG("Generating config to " << output_file);
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

        out << "namespace coco {\n";
        out << "[[nodiscard]] inline std::string read_rule(const std::string &path)\n{\n";
        out << "    std::ifstream in(path);\n";
        out << "    if (!in)\n";
        out << "        throw std::runtime_error(\"Cannot open rule file: \" + path);\n";
        out << "    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());\n";
        out << "}\n\n";

        out << "inline void config(coco &cc)\n{\n";
        generate_types(out);
        generate_rules(out);
        out << "}\n";
        out << "} // namespace coco\n";
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
