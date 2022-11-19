#pragma once

#include "json.h"
#include "clips.h"

namespace coco
{
  class sensor_type
  {
    friend class coco;

  public:
    sensor_type(const std::string &id, const std::string &name, const std::string &description, Fact *fact) : id(id), name(name), description(description), fact(fact) {}
    ~sensor_type() = default;

    const std::string &get_id() const { return id; }
    const std::string &get_name() const { return name; }
    const std::string &get_description() const { return description; }
    Fact *get_fact() const { return fact; }

  private:
    const std::string id;
    const std::string name;
    const std::string description;
    Fact *fact;
  };
} // namespace coco
