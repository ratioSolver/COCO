#include "config_generator.hpp"
#include "logging.hpp"

enum ParType
{
    Type,
    Rule,
    Item,
    Output
};

int main(int argc, char const *argv[])
{
    std::vector<std::string> type_files;
    std::vector<std::string> rule_files;
    std::vector<std::string> items_files;
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
        else if (std::string(argv[i]) == "-i")
        {
            current_par = Item;
            continue;
        }

        switch (current_par)
        {
        case Type:
            type_files.push_back(argv[i]);
            break;
        case Rule:
            rule_files.push_back(argv[i]);
            break;
        case Item:
            items_files.push_back(argv[i]);
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
        coco::config_generator gen(type_files, rule_files, items_files, output);
        gen.generate_config();
    }
    catch (const std::exception &e)
    {
        LOG_ERR("Error during config generation: " << e.what());
        return 1;
    }

    return 0;
}
