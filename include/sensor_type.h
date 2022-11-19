#pragma once

#include "json.h"
#include "clips.h"

namespace coco
{
  class coco;
  class coco_db;

  class sensor_type
  {
    friend class coco;
    friend class coco_db;

  public:
    sensor_type(const std::string &id, const std::string &name, const std::string &description) : id(id), name(name), description(description) {}
    ~sensor_type() = default;

    const std::string &get_id() const { return id; }
    const std::string &get_name() const { return name; }
    const std::string &get_description() const { return description; }
    Fact *get_fact() const { return fact; }

  private:
    const std::string id;
    std::string name;
    std::string description;
    Fact *fact = nullptr;
  };
} // namespace coco
