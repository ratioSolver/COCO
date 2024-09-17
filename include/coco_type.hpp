#pragma once

#include "coco_property.hpp"
#include <set>

namespace coco
{
  class coco_db;

  /**
   * @brief Represents a type in the coco database.
   *
   * The `type` class stores information about a type, including its ID, name, description, and properties.
   * It provides methods to access and retrieve this information.
   */
  class type final
  {
    friend class coco_db;

  public:
    /**
     * @brief Constructs a new `type` object.
     *
     * @param cc The CoCo core object.
     * @param id The ID of the type.
     * @param name The name of the type.
     * @param description The description of the type.
     * @param props The properties of the type.
     * @param parents The parent types of the type.
     * @param static_properties The static properties of the type.
     * @param dynamic_properties The dynamic properties of the type.
     */
    type(coco_core &cc, const std::string &id, const std::string &name, const std::string &description, json::json &&props = json::json(), std::vector<std::reference_wrapper<const type>> &&parents = {}, std::vector<std::unique_ptr<property>> &&static_properties = {}, std::vector<std::unique_ptr<property>> &&dynamic_properties = {}) noexcept;
    ~type() noexcept;

    /**
     * @brief Gets the ID of the type.
     *
     * @return The ID of the type.
     */
    std::string get_id() const noexcept { return id; }

    /**
     * @brief Gets the name of the type.
     *
     * @return The name of the type.
     */
    std::string get_name() const noexcept { return name; }

    /**
     * @brief Gets the description of the type.
     *
     * @return The description of the type.
     */
    std::string get_description() const noexcept { return description; }

    /**
     * @brief Gets the properties of the type.
     *
     * @return The properties of the type.
     */
    const json::json &get_properties() const noexcept { return properties; }

    /**
     * @brief Gets the parent types of the type.
     *
     * @return The parent types of the type.
     */
    const std::map<std::string, std::reference_wrapper<const type>> &get_parents() const noexcept { return parents; }

    /**
     * @brief Gets the static properties of the type.
     *
     * @return The static properties of the type.
     */
    const std::map<std::string, std::unique_ptr<property>> &get_static_properties() const noexcept { return static_properties; }

    /**
     * @brief Gets the dynamic properties of the type.
     *
     * @return The dynamic properties of the type.
     */
    const std::map<std::string, std::unique_ptr<property>> &get_dynamic_properties() const noexcept { return dynamic_properties; }

  private:
    void set_name(const std::string &name) noexcept;
    void set_description(const std::string &description) noexcept;
    /**
     * @brief Sets the properties of the type.
     *
     * This function takes a JSON object containing the properties and sets them for the type.
     *
     * @param props The JSON object containing the properties.
     */
    void set_properties(json::json &&props) noexcept;
    void add_parent(const type &parent) noexcept;
    void remove_parent(const type &parent) noexcept;
    void add_static_property(std::unique_ptr<property> &&prop) noexcept;
    void remove_static_property(const property &prop) noexcept;
    void add_dynamic_property(std::unique_ptr<property> &&prop) noexcept;
    void remove_dynamic_property(const property &prop) noexcept;

  private:
    coco_core &cc;                                                       // The CoCo core object.
    Fact *type_fact = nullptr;                                           // The type fact.
    std::string id;                                                      // The ID of the type.
    std::string name;                                                    // The name of the type.
    std::string description;                                             // The description of the type.
    std::map<std::string, std::reference_wrapper<const type>> parents;   // The parent types of the type.
    std::map<std::string, Fact *> parent_facts;                          // The parent facts of the type.
    json::json properties;                                               // The properties of the type.
    std::map<std::string, std::unique_ptr<property>> static_properties;  // The static properties of the type.
    std::map<std::string, std::unique_ptr<property>> dynamic_properties; // The dynamic properties of the type.
  };
} // namespace coco
