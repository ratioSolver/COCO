#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#include <chrono>
#include <unordered_map>

namespace coco
{
  class coco;
  class item;

  class type
  {
  public:
    /**
     * @brief Constructs a new `type` object.
     *
     * @param cc The CoCo object.
     * @param name The name of the type.
     * @param static_props The static properties of the type.
     * @param dynamic_props The dynamic properties of the type.
     * @param data The (optional) data of the type.
     */
    type(coco &cc, std::string_view name, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json()) noexcept;
    ~type();

    /**
     * @brief Gets the name of the type.
     *
     * @return The name of the type.
     */
    const std::string &get_name() const noexcept { return name; }

    /**
     * @brief Gets the data of the type.
     *
     * @return The data of the type.
     */
    const json::json &get_data() const noexcept { return data; }

    item &new_item(std::string_view id, const type &tp, json::json &&props = json::json(), json::json &&val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) noexcept;

  private:
    coco &cc;                                                      // The CoCo object..
    Fact *type_fact = nullptr;                                     // The type fact..
    std::string name;                                              // The name of the type..
    const json::json static_properties;                            // The static properties..
    const json::json dynamic_properties;                           // The dynamic properties..
    const json::json data;                                         // The data of the type..
    std::unordered_map<std::string, utils::u_ptr<item>> instances; // the instances of the type..
  };
} // namespace coco
