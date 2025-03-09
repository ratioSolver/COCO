#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#include <chrono>

namespace coco
{
  class type;

  class item
  {
  public:
    /**
     * @brief Constructs an item object.
     *
     * @param id The ID of the item.
     * @param tp The type of the item.
     * @param props The properties of the item.
     * @param val The value of the item.
     * @param timestamp The timestamp of the value.
     */
    item(std::string_view id, const type &tp, json::json &&props = json::json(), json::json &&val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) noexcept;
    ~item() noexcept;

  private:
    const std::string id;                                   // The ID of the item.
    const type &tp;                                         // The type of the item.
    json::json static_properties;                           // The static properties of the item.
    json::json dynamic_properties;                          // The dynamic properties of the item.
    std::chrono::system_clock::time_point timestamp;        // The timestamp of the value.
    Fact *item_fact = nullptr;                              // The fact representing the item.
    Fact *is_instance_of = nullptr;                         // The fact representing the type of the item.
    std::map<std::string, Fact *> static_properties_facts;  // The facts representing the static properties of the item.
    std::map<std::string, Fact *> dynamic_properties_facts; // The facts representing the dynamic properties of the item.
  };
} // namespace coco
