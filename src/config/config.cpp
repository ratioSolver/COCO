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

        std::string tp_name = j_t["name"].get<std::string>();
        std::string static_props = j_t.contains("static_properties") ? "json::load(R\"(" + j_t["static_properties"].dump() + ")\")" : "{}";
        std::string dynamic_props = j_t.contains("dynamic_properties") ? "json::load(R\"(" + j_t["dynamic_properties"].dump() + ")\")" : "{}";
        std::string data = j_t.contains("data") ? "json::load(R\"(" + j_t["data"].dump() + ")\")" : "{}";
        out << "    try {\n";
        out << "        [[maybe_unused]] auto &" << tp_name << " = cc.get_type(\"" << tp_name << "\");\n";
        out << "        LOG_DEBUG(\"Type `" << tp_name << "` found\");\n";
        out << "    } catch (const std::invalid_argument &e) {\n";
        out << "        LOG_DEBUG(\"Creating `" << tp_name << "` type\");\n";
        out << "        [[maybe_unused]] auto &" << tp_name << " = cc.create_type(\"" << tp_name << "\", " << static_props << ", " << dynamic_props << ", " << data << ");\n";
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
