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
     * @brief Gets the parent types of the type.
     *
     * @return The parent types of the type.
     */
    [[nodiscard]] const std::map<std::string, utils::ref_wrapper<const type>> &get_parents() const noexcept { return parents; }

    /**
     * @brief Sets the parents of the type.
     *
     * @param parents The parents of the type.
     */
    void set_parents(std::vector<utils::ref_wrapper<const type>> &&parents) noexcept;

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
    [[nodiscard]] const json::json &get_static_properties() const noexcept { return static_properties; }

    /**
     * @brief Gets the dynamic properties of the type.
     *
     * @return The dynamic properties of the type.
     */
    [[nodiscard]] const json::json &get_dynamic_properties() const noexcept { return dynamic_properties; }

    /**
     * @brief Creates a new instance of the type.
     *
     * @param id The id of the instance.
     * @param props The properties of the new instance.
     * @param val The current value of the new instance.
     * @param timestamp The timestamp of the value.
     * @return The new instance of the type.
     */
    [[nodiscard]] item &new_instance(std::string_view id, json::json &&props = json::json(), json::json &&val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) noexcept;

  private:
    coco &cc;                                                      // The CoCo object..
    std::string name;                                              // The name of the type..
    std::map<std::string, utils::ref_wrapper<const type>> parents; // The parent types of the type.
    const json::json data;                                         // The data of the type..
    const json::json static_properties;                            // The static properties..
    const json::json dynamic_properties;                           // The dynamic properties..
    Fact *type_fact = nullptr;                                     // The type fact..
    std::map<std::string, Fact *> parent_facts;                    // The parent facts of the type.
    std::unordered_map<std::string, utils::u_ptr<item>> instances; // the instances of the type by their ID..
  };
} // namespace coco
