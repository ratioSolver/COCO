#include "config_generator.hpp"
#include "json.hpp"
#include "logging.hpp"
#include <regex>

namespace coco
{
    config_generator::config_generator(const std::vector<std::string> &type_files, const std::vector<std::string> &rule_files, const std::vector<std::string> &items_files, const std::string &output_file) : output(output_file), config_file(output_file)
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

        LOG_DEBUG("Loading items");
        for (const auto &it_file : items_files)
        {
            LOG_DEBUG("Loading " << it_file);
            std::ifstream in(it_file);
            if (!in)
                throw std::runtime_error("Cannot open items file: " + it_file);
            json::json j_it = json::load(in);
            std::filesystem::path it_path(it_file);
            auto it_name = it_path.stem().string();
            items[it_name] = j_it;
            LOG_DEBUG("Loaded item: " << it_name);
        }

        try
        {
            auto parent = output.parent_path();
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
    }

    void config_generator::generate_types(std::ofstream &out)
    {
        out << "    std::vector<db_type> types;\n";
        for (const auto &[tp_name, j_t] : types)
        {
            auto cpp_tp_name = to_cpp_identifier(tp_name);
            out << "    std::optional<json::json> " << cpp_tp_name << "_static_props, " << cpp_tp_name << "_dynamic_props, " << cpp_tp_name << "_data;\n";
            if (j_t.contains("static_properties"))
                out << "    " << cpp_tp_name << "_static_props = json::load(R\"(" << j_t["static_properties"].dump() << ")\");\n";
            if (j_t.contains("dynamic_properties"))
                out << "    " << cpp_tp_name << "_dynamic_props = json::load(R\"(" << j_t["dynamic_properties"].dump() << ")\");\n";
            if (j_t.contains("data"))
                out << "    " << cpp_tp_name << "_data = json::load(R\"(" << j_t["data"].dump() << ")\");\n";
            out << "    types.push_back(db_type{\"" << tp_name << "\", " << cpp_tp_name << "_static_props, " << cpp_tp_name << "_dynamic_props, " << cpp_tp_name << "_data});\n";
        }
        out << "\n";

        out << "    for (auto it = types.begin(); it != types.end(); )\n";
        out << "        try {\n";
        out << "            [[maybe_unused]] auto &" << "tp = cc.get_type(it->name);\n";
        out << "            LOG_DEBUG(\"Type `\" << it->name << \"` found\");\n";
        out << "            it = types.erase(it);\n";
        out << "        } catch (const std::invalid_argument &e) {\n";
        out << "            LOG_DEBUG(\"Creating `\" << it->name << \"` type\");\n";
        out << "            ++it;\n";
        out << "        }\n";
        out << "    set_types(cc, std::move(types));\n";
        out << "\n";
    }

    void config_generator::generate_rules(std::ofstream &out)
    {
        out << "    cc.load_rules();\n";
        for (const auto &[r_name, r_content] : rules)
        {
            LOG_DEBUG("Generating rule: " << r_name);
            out << "    try {\n";
            out << "        [[maybe_unused]] auto &" << r_name << "_rule = cc.get_reactive_rule(\"" << r_name << "\");\n";
            out << "        LOG_DEBUG(\"Reactive rule `" << r_name << "` found\");\n";
            out << "    } catch (const std::invalid_argument &e) {\n";
            out << "        LOG_DEBUG(\"Creating `" << r_name << "` reactive rule\");\n";
            out << "        [[maybe_unused]] auto &" << r_name << "_rule = cc.create_reactive_rule(\"" << r_name << "\", R\"(" << r_content << ")\");\n";
            out << "    }\n";
        }
    }

    void config_generator::generate_items(std::ofstream &out)
    {
        out << "    auto itms = cc.get_items();\n";
        out << "    if (itms.empty())\n";
        out << "    {\n";
        out << "        LOG_WARN(\"No items found in the database. Creating default items.\");\n";
        for (const auto &[it_name, j_it] : items)
        {
            LOG_DEBUG("Generating item: " << it_name);
            auto cpp_it_name = to_cpp_identifier(it_name);
            out << "        std::vector<std::reference_wrapper<type>> " << cpp_it_name << "_types;\n";
            for (const auto &tp_name : j_it["types"].as_array())
                out << "        " << cpp_it_name << "_types.push_back(cc.get_type(\"" << tp_name.get<std::string>() << "\"));\n";
            out << "        [[maybe_unused]] auto &" << cpp_it_name << " = cc.create_item(std::move(" << cpp_it_name << "_types));\n";
            out << "\n";
        }
        for (const auto &[it_name, j_it] : items)
        {
            if (!j_it.contains("properties"))
                continue;
            LOG_DEBUG("Setting properties for item: " << it_name);
            auto cpp_it_name = to_cpp_identifier(it_name);
            out << "        json::json " << cpp_it_name << "_props = json::load(R\"(" << j_it["properties"].dump() << ")\");\n";
            for (const auto &[prop_name, prop_value] : j_it["properties"].as_object())
                if (prop_value.is_object() && prop_value.contains("item"))
                {
                    auto ref_it_name = prop_value["item"].get<std::string>();
                    auto cpp_ref_it_name = to_cpp_identifier(ref_it_name);
                    out << "        " << cpp_it_name << "_props[\"" << prop_name << "\"] = " << cpp_ref_it_name << ".get_id();\n";
                }
            out << "        cc.set_properties(" << cpp_it_name << ", std::move(" << cpp_it_name << "_props));\n";
            out << "\n";
        }
        out << "        LOG_INFO(\"Default items created.\");\n";
        out << "    }\n";
    }

    void config_generator::generate_messages()
    {
        for (const auto &[name, j_tp] : types)
        {
            std::ofstream msg_file((output.parent_path() / (name + ".msg")).string(), std::ios::out | std::ios::trunc);
            if (!msg_file)
            {
                LOG_ERR("Cannot create message file: " << name << ".msg");
                continue;
            }
            LOG_DEBUG("Generating message file for type: " << name);
            if (j_tp.contains("static_properties"))
                for (const auto &[prop_name, prop_value] : j_tp["static_properties"].as_object())
                    msg_file << prop_to_ros(prop_name, prop_value);
        }
    }

    void config_generator::generate_config()
    {
        LOG_DEBUG("Generating config to " << config_file);
        std::ofstream out(config_file, std::ios::out | std::ios::trunc);
        if (!out)
        {
            LOG_ERR("Cannot open output file: " << config_file);
            return;
        }

        out << "// This file is auto-generated. Do not edit manually.\n\n";
        out << "#pragma once\n\n";
        out << "#include \"coco.hpp\"\n";
        out << "#include \"coco_db.hpp\"\n";
        out << "#include \"coco_item.hpp\"\n";
        out << "#include \"logging.hpp\"\n";
        out << "#include <fstream>\n\n";

        out << "namespace coco {\n";
        out << "inline void config(coco &cc)\n{\n";
        generate_types(out);
        generate_rules(out);
        out << "\n";
        generate_items(out);
        out << "}\n";
        out << "} // namespace coco\n";

        LOG_DEBUG("Generating message files");
        generate_messages();
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

    std::string config_generator::prop_to_ros(const std::string &name, const json::json &prop)
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
