#pragma once

#include "json.hpp"
#include "clips.h"
#include <chrono>
#include <optional>
#include <unordered_set>
#include <memory>

namespace coco
{
  class coco;
  class property;
  class item;

  class type final
  {
    friend class coco;

  public:
    /**
     * @brief Constructs a new `type` object.
     *
     * @param cc The CoCo object.
     * @param name The name of the type.
     * @param data The (optional) data of the type.
     */
    type(coco &cc, std::string_view name, json::json &&data = json::json()) noexcept;
    ~type();

    /**
     * @brief Gets the CoCo environment of the item.
     *
     * @return The CoCo environment of the item.
     */
    [[nodiscard]] coco &get_coco() const noexcept { return cc; }

    /**
     * @brief Gets the name of the type.
     *
     * @return The name of the type.
     */
    [[nodiscard]] const std::string &get_name() const noexcept { return name; }

    /**
     * @brief Gets the data of the type.
     *
     * @return The data of the type.
     */
    [[nodiscard]] const json::json &get_data() const noexcept { return data; }

    /**
     * @brief Gets the static properties of the type.
     *
     * @return The static properties of the type.
     */
    [[nodiscard]] const std::map<std::string, std::unique_ptr<property>> &get_static_properties() const noexcept { return static_properties; }

    /**
     * @brief Gets the dynamic properties of the type.
     *
     * @return The dynamic properties of the type.
     */
    [[nodiscard]] const std::map<std::string, std::unique_ptr<property>> &get_dynamic_properties() const noexcept { return dynamic_properties; }

    void set_properties(json::json &&static_props, json::json &&dynamic_props) noexcept;

    /**
     * @brief Gets the instances of the type.
     *
     * @return The instances of the type.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<item>> get_instances() const noexcept;

    /**
     * @brief Adds an instance to the type.
     *
     * @param itm The instance to add.
     */
    void add_instance(item &itm) noexcept;
    /**
     * @brief Removes an instance from the type.
     *
     * @param itm The instance to remove.
     */
    void remove_instance(item &itm) noexcept;

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    coco &cc;                                                            // The CoCo object..
    std::string name;                                                    // The name of the type..
    const json::json data;                                               // The data of the type..
    std::map<std::string, std::unique_ptr<property>> static_properties;  // The static properties..
    std::map<std::string, std::unique_ptr<property>> dynamic_properties; // The dynamic properties..
    std::unordered_set<std::string> instances;                           // The IDs of the instances of the type..
  };
} // namespace coco
