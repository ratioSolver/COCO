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
        out << "    std::vector<db_type> types;\n";
        for (const auto &[tp_name, j_t] : types)
        {
            out << "    std::optional<json::json> " << tp_name << "_static_props, " << tp_name << "_dynamic_props, " << tp_name << "_data;\n";
            if (j_t.contains("static_properties"))
                out << "    " << tp_name << "_static_props = json::load(R\"(" << j_t["static_properties"].dump() << ")\");\n";
            if (j_t.contains("dynamic_properties"))
                out << "    " << tp_name << "_dynamic_props = json::load(R\"(" << j_t["dynamic_properties"].dump() << ")\");\n";
            if (j_t.contains("data"))
                out << "    " << tp_name << "_data = json::load(R\"(" << j_t["data"].dump() << ")\");\n";
            out << "    types.push_back(db_type{\"" << tp_name << "\", " << tp_name << "_static_props, " << tp_name << "_dynamic_props, " << tp_name << "_data});\n";
        }

        out << "    for (auto it = types.begin(); it != types.end(); )\n";
        out << "        try {\n";
        out << "            [[maybe_unused]] auto &" << "tp = cc.get_type(it->name);\n";
        out << "            LOG_DEBUG(\"Type `\" << it->name << \"` found\");\n";
        out << "            it = types.erase(it);\n";
        out << "        } catch (const std::invalid_argument &e) {\n";
        out << "            LOG_DEBUG(\"Creating `\" << it->name << \"` type\");\n";
        out << "            ++it;\n";
        out << "        }\n";
        out << "    cc.make_types(std::move(types));\n";
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
        out << "#include \"coco_db.hpp\"\n";
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
