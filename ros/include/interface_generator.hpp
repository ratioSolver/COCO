#pragma once

#include "json.hpp"
#include <filesystem>
#include <unordered_map>

namespace coco
{
  class interface_generator
  {
  public:
    interface_generator(std::vector<std::filesystem::path> &&type_files, std::filesystem::path &&output_dir);

    void generate_messages();
    void generate_package_xml();
    void generate_cmake_lists();

  protected:
    static std::string to_ros_identifier(const std::string &symbol) noexcept;
    static std::string prop_to_ros(const std::string &name, const json::json &prop);

  private:
    std::unordered_map<std::string, json::json> types;
    std::filesystem::path output_dir;
  };
} // namespace coco
