#pragma once

#include "json.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <filesystem>

namespace coco
{
  class config_generator
  {
  public:
    config_generator(const std::vector<std::string> &type_files, const std::vector<std::string> &rule_files, const std::vector<std::string> &items_files, const std::string &output_file);

    void generate_config();

  private:
    void generate_types(std::ofstream &out);
    void generate_rules(std::ofstream &out);
    void generate_items(std::ofstream &out);

    void generate_messages();
    void generate_package_xml();

    static std::string to_cpp_identifier(const std::string &symbol);
    static std::string prop_to_ros(const std::string &name, const json::json &prop);

  private:
    std::unordered_map<std::string, json::json> types;
    std::unordered_map<std::string, std::string> rules;
    std::unordered_map<std::string, json::json> items;
    std::filesystem::path output;
    std::string config_file;
  };
} // namespace coco
