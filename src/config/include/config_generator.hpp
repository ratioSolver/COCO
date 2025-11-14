#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace coco
{
  class config_generator
  {
  public:
    static void generate_config(const std::vector<std::string> &type_files, const std::vector<std::string> &rule_files, const std::string &output_file);

  private:
    static void generate_types(const std::vector<std::string> &type_files, std::ofstream &out);
    static void generate_rules(const std::vector<std::string> &rule_files, std::ofstream &out);

    static std::string to_cpp_identifier(const std::string &symbol);
  };
} // namespace coco
