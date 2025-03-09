#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#include <string>

namespace coco
{
  class coco;

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

  private:
    coco &cc;                            // The CoCo object..
    Fact *type_fact = nullptr;           // The type fact..
    std::string name;                    // The name of the type..
    const json::json static_properties;  // The static properties..
    const json::json dynamic_properties; // The dynamic properties..
    const json::json data;               // The data of the type..
  };
} // namespace coco
