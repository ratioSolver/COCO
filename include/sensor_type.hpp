#pragma once

#include "parameter.hpp"
#include "json.hpp"
#include "clips.h"
#include <memory>
#include <limits>

namespace coco
{
  class sensor_type
  {
  public:
    sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars);
    virtual ~sensor_type() = default;

    [[nodiscard]] const std::string &get_id() const { return id; }
    [[nodiscard]] const std::string &get_name() const { return name; }
    [[nodiscard]] const std::string &get_description() const { return description; }
    [[nodiscard]] const std::vector<std::unique_ptr<parameter>> &get_parameters() const { return parameters; }

  private:
    const std::string id;
    std::string name, description;
    std::vector<std::unique_ptr<parameter>> parameters;
  };
} // namespace coco
