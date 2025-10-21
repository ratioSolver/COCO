#include "json.hpp"
#include "logging.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <functional>

enum ParType
{
    Type,
    Rule,
    Output
};

int main(int argc, char const *argv[])
{
    std::vector<std::string> types;
    std::vector<std::string> rules;
    std::string output;

    ParType current_par = Type;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "-t")
        {
            current_par = Type;
            continue;
        }
        else if (std::string(argv[i]) == "-r")
        {
            current_par = Rule;
            continue;
        }
        else if (std::string(argv[i]) == "-o")
        {
            current_par = Output;
            continue;
        }

        switch (current_par)
        {
        case Type:
            types.push_back(argv[i]);
            break;
        case Rule:
            rules.push_back(argv[i]);
            break;
        case Output:
            output = argv[i];
            break;
        }
    }

    LOG_DEBUG("Output file: " << output);
    if (output.empty())
    {
        LOG_ERR("No output file specified");
        return 1;
    }

    try
    {
        std::filesystem::path outp(output);
        auto parent = outp.parent_path();
        if (!parent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec)
            {
                LOG_ERR("Cannot create directories for output: " << parent << " : " << ec.message());
                return 1;
            }
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERR("Filesystem error while preparing output path: " << e.what());
        return 1;
    }

    std::ofstream out(output, std::ios::out | std::ios::trunc);
    if (!out)
    {
        LOG_ERR("Cannot open output file: " << output);
        return 1;
    }

    std::map<std::string, json::json> tps_map;
    for (const auto &tp : types)
    {
        LOG_DEBUG("Loading " << tp);
        std::ifstream in(tp);
        if (!in)
        {
            LOG_ERR("Cannot open type file: " << tp);
            return 1;
        }
        json::json j_t = json::load(in);
        LOG_DEBUG("Loaded type: " << j_t.dump());
        tps_map[j_t["name"].get<std::string>()] = std::move(j_t);
    }

    std::set<std::string> visited_types;
    std::vector<std::string> generation_order;

    out << "// This file is auto-generated. Do not edit manually.\n\n";
    out << "#pragma once\n\n";
    out << "#include \"coco.hpp\"\n";
    out << "#include \"logging.hpp\"\n";
    out << "#include <fstream>\n\n";

    out << "[[nodiscard]] std::string read_rule(const std::string &path)\n{\n";
    out << "    std::ifstream in(path);\n";
    out << "    if (!in)\n";
    out << "        throw std::runtime_error(\"Cannot open rule file: \" + path);\n";
    out << "    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());\n";
    out << "}\n\n";

    out << "void load_config(coco::coco &cc)\n{\n";

    std::function<void(const std::string &)> visit = [&](const std::string &name)
    {
        if (visited_types.count(name))
            return;
        if (tps_map.at(name).contains("parents"))
            for (const auto &parent : tps_map.at(name)["parents"].as_array())
                visit(parent.get<std::string>());
        visited_types.insert(name);
        generation_order.push_back(name);
    };

    for (const auto &tp_pair : tps_map)
        visit(tp_pair.first);

    for (const auto &tp_name : generation_order)
    {
        out << "    try {\n";
        out << "        [[maybe_unused]] auto &" << tp_name << " = cc.get_type(\"" << tp_name << "\");\n";
        out << "        LOG_DEBUG(\"Type `" << tp_name << "` found\");\n";
        out << "    } catch (const std::invalid_argument &e) {\n";
        out << "        LOG_DEBUG(\"Creating `" << tp_name << "` type\");\n";
        std::string parents;
        if (tps_map.at(tp_name).contains("parents"))
        {
            out << "        std::vector<std::reference_wrapper<const coco::type>> " << tp_name << "_parents;\n";
            for (const auto &parent : tps_map.at(tp_name)["parents"].as_array())
                out << "        " << tp_name << "_parents.emplace_back(cc.get_type(\"" << parent.get<std::string>() << "\"));\n";
            parents = "std::move(" + tp_name + "_parents)";
        }
        else
            parents = "{}";
        std::string static_props = tps_map.at(tp_name).contains("static_properties") ? "json::load(R\"(" + tps_map.at(tp_name)["static_properties"].dump() + ")\")" : "{}";
        std::string dynamic_props = tps_map.at(tp_name).contains("dynamic_properties") ? "json::load(R\"(" + tps_map.at(tp_name)["dynamic_properties"].dump() + ")\")" : "{}";
        std::string data = tps_map.at(tp_name).contains("data") ? "json::load(R\"(" + tps_map.at(tp_name)["data"].dump() + ")\")" : "{}";
        out << "        [[maybe_unused]] auto &" << tp_name << " = cc.create_type(\"" << tp_name << "\", " << parents << ", " << static_props << ", " << dynamic_props << ", " << data << ");\n";
        out << "    }\n";
    }

    out << "    cc.load_rules();\n";

    for (const auto &rp : rules)
    {
        LOG_DEBUG("Loading " << rp);
        std::ifstream in(rp);
        if (!in)
        {
            LOG_ERR("Cannot open rule file: " << rp);
            return 1;
        }
        std::filesystem::path rp_path(rp);
        std::string name_no_ext = rp_path.stem().string();
        out << "    try {\n";
        out << "        [[maybe_unused]] auto &" << name_no_ext << "_rule = cc.get_reactive_rule(\"" << name_no_ext << "\");\n";
        out << "        LOG_DEBUG(\"Reactive rule `" << name_no_ext << "` found\");\n";
        out << "    } catch (const std::invalid_argument &e) {\n";
        out << "        LOG_DEBUG(\"Creating `" << name_no_ext << "` reactive rule\");\n";
        out << "        [[maybe_unused]] auto &" << name_no_ext << "_rule = cc.create_reactive_rule(\"" << name_no_ext << "\", read_rule(\"" << rp << "\"));\n";
        out << "    }\n";
    }
    out << "}\n";

    return 0;
}
