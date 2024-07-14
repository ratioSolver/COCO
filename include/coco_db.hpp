#pragma once

#include "coco_type.hpp"
#include "coco_item.hpp"
#include "coco_rule.hpp"
#include <chrono>
#include <stdexcept>

namespace coco
{
  class coco_db
  {
  public:
    coco_db(const json::json &config = {});
    virtual ~coco_db() = default;

    const json::json &get_config() const { return config; }

    /**
     * @brief Creates a new type.
     *
     * This function creates a new type with the specified name, description, and properties.
     *
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_properties The static properties of the type.
     * @param dynamic_properties The dynamic properties of the type.
     * @return A reference to the created type.
     */
    virtual type &create_type(const std::string &name, const std::string &description, std::map<std::string, std::unique_ptr<property>> &&static_properties, std::map<std::string, std::unique_ptr<property>> &&dynamic_properties) = 0;

    /**
     * Sets the name of the given type.
     *
     * @param tp The type to set the name for.
     * @param name The name to set for the type.
     */
    virtual void set_type_name(type &tp, const std::string &name) { tp.name = name; }

    /**
     * @brief Sets the description of a type.
     *
     * This function sets the description of a given type. The description provides additional information about the type.
     *
     * @param tp The type to set the description for.
     * @param description The description to set for the type.
     */
    virtual void set_type_description(type &tp, const std::string &description) { tp.description = description; }

    /**
     * Adds a static property to the given type.
     *
     * @param tp The type to which the static property will be added.
     * @param prop The static property to add.
     */
    virtual void add_static_property(type &tp, std::unique_ptr<property> &&prop) { tp.static_properties.emplace(prop->get_name(), std::move(prop)); }

    /**
     * @brief Removes a static property from the given type.
     *
     * This function removes the static property with the specified id from the given type.
     *
     * @param tp The type from which to remove the static property.
     * @param prop The static property to remove.
     */
    virtual void remove_static_property(type &tp, const property &prop) { tp.static_properties.erase(prop.get_name()); }

    /**
     * Adds a dynamic property to the given type.
     *
     * @param tp The type to which the dynamic property will be added.
     * @param prop The dynamic property to add.
     */
    virtual void add_dynamic_property(type &tp, std::unique_ptr<property> &&prop) { tp.dynamic_properties.emplace(prop->get_name(), std::move(prop)); }

    /**
     * @brief Removes a dynamic property from the given type.
     *
     * This function removes the dynamic property with the specified id from the given type.
     *
     * @param tp The type from which to remove the dynamic property.
     * @param prop The dynamic property to remove.
     */
    virtual void remove_dynamic_property(type &tp, const property &prop) { tp.dynamic_properties.erase(prop.get_name()); }

    /**
     * @brief Removes a type from the database.
     *
     * This function removes the specified type from the database.
     *
     * @param tp The type to be removed.
     */
    virtual void delete_type(const type &tp)
    {
      types_by_name.erase(tp.name);
      types_by_id.erase(tp.id);
    }

    /**
     * Returns a vector of references to the types.
     *
     * This function retrieves all the types stored in the database and returns them as a vector of `type` objects. The returned vector contains references to the actual types stored in the `types` map.
     *
     * @return A vector of types.
     */
    std::vector<std::reference_wrapper<type>> get_types() const
    {
      std::vector<std::reference_wrapper<type>> res;
      for (auto &s : types_by_id)
        res.push_back(*s.second);
      return res;
    }

    /**
     * Retrieves the reference to a type based on its ID.
     *
     * @param id The ID of the type to retrieve.
     * @return A reference to the type with the specified ID.
     * @throws std::invalid_argument if the type with the specified ID is not found.
     */
    type &get_type(const std::string &id) const
    {
      if (types_by_id.find(id) == types_by_id.end())
        throw std::invalid_argument("Type not found: " + id);
      return *types_by_id.at(id);
    }

    /**
     * Retrieves the reference to a type based on its name.
     *
     * @param name The name of the type to retrieve.
     * @return A reference to the type with the specified name.
     * @throws std::invalid_argument if the type with the specified name is not found.
     */
    type &get_type_by_name(const std::string &name) const
    {
      if (types_by_name.find(name) == types_by_name.end())
        throw std::invalid_argument("Type not found: " + name);
      return types_by_name.at(name);
    }

    /**
     * @brief Creates an item of the specified type with the given name and optional data.
     *
     * @param tp The type of the item.
     * @param name The name of the item.
     * @param pars The properties of the item.
     * @return A reference to the created item.
     */
    virtual item &create_item(const type &tp, const std::string &name, const json::json &pars) = 0;

    /**
     * Sets the name of an item.
     *
     * @param itm The item to set the name for.
     * @param name The new name for the item.
     */
    virtual void set_item_name(item &itm, const std::string &name) { itm.name = name; }
    /**
     * @brief Sets the properties of an item.
     *
     * This function sets the properties of the given item using the provided JSON object.
     *
     * @param itm The item to set the properties for.
     * @param props The JSON object containing the properties.
     */
    virtual void set_item_properties(item &itm, const json::json &props) { itm.properties = props; }

    /**
     * @brief Removes an item from the database.
     *
     * This function removes the specified item from the database.
     *
     * @param itm The item to be removed.
     */
    virtual void delete_item(const item &itm) { items.erase(itm.id); }

    /**
     * Retrieves a vector of references to the items in the database.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of references to the items.
     */
    std::vector<std::reference_wrapper<item>> get_items() const
    {
      std::vector<std::reference_wrapper<item>> res;
      for (auto &s : items)
        res.push_back(*s.second);
      return res;
    }

    /**
     * Retrieves the reference to an item based on its ID.
     *
     * @param id The ID of the item to retrieve.
     * @return A reference to the item with the specified ID.
     * @throws std::invalid_argument if the item with the specified ID is not found.
     */
    item &get_item(const std::string &id) const
    {
      if (items.find(id) == items.end())
        throw std::invalid_argument("Item not found: " + id);
      return *items.at(id);
    }

    /**
     * @brief Adds data to the database for a given item.
     *
     * This function adds the provided data to the database for the specified item.
     *
     * @param itm The item to which the data will be added.
     * @param timestamp The timestamp of the data.
     * @param data The data to be added to the database.
     */
    virtual void add_data(const item &itm, const std::chrono::system_clock::time_point &timestamp, const json::json &data) = 0;

    /**
     * Returns a vector of references to the reactive rules in the database.
     *
     * @return A vector of references to the reactive rules.
     */
    std::vector<std::reference_wrapper<rule>> get_reactive_rules() const
    {
      std::vector<std::reference_wrapper<rule>> res;
      for (auto &r : reactive_rules)
        res.push_back(*r.second);
      return res;
    }

    /**
     * @brief Creates a reactive rule with the given name and content.
     *
     * @param name The name of the reactive rule.
     * @param content The content of the reactive rule.
     * @return A reference to the created reactive rule.
     */
    virtual rule &create_reactive_rule(const std::string &name, const std::string &content) = 0;

    /**
     * Returns a vector of references to the deliberative rules in the database.
     *
     * @return A vector of references to the deliberative rules.
     */
    std::vector<std::reference_wrapper<rule>> get_deliberative_rules() const
    {
      std::vector<std::reference_wrapper<rule>> res;
      for (auto &r : deliberative_rules)
        res.push_back(*r.second);
      return res;
    }

    /**
     * @brief Creates a deliberative rule with the given name and content.
     *
     * @param name The name of the deliberative rule.
     * @param content The content of the deliberative rule.
     * @return A reference to the created deliberative rule.
     */
    virtual rule &create_deliberative_rule(const std::string &name, const std::string &content) = 0;

  protected:
    type &create_type(const std::string &id, const std::string &name, const std::string &description, std::map<std::string, std::unique_ptr<property>> &&static_properties, std::map<std::string, std::unique_ptr<property>> &&dynamic_properties)
    {
      if (types_by_id.find(id) != types_by_id.end())
        throw std::invalid_argument("Type already exists: " + id);
      auto tp = std::make_unique<type>(id, name, description, std::move(static_properties), std::move(dynamic_properties));
      types_by_name.emplace(name, *tp);
      types_by_id.emplace(id, std::move(tp));
      return *types_by_id[id];
    }
    item &create_item(const std::string &id, const type &tp, const std::string &name, const json::json &pars)
    {
      if (items.find(id) != items.end())
        throw std::invalid_argument("item already exists: " + id);
      items.emplace(id, std::make_unique<item>(id, tp, name, pars));
      items[id] = std::make_unique<item>(id, tp, name, pars);
      return *items[id];
    }
    rule &create_reactive_rule(const std::string &id, const std::string &name, const std::string &content)
    {
      if (reactive_rules.find(id) != reactive_rules.end())
        throw std::invalid_argument("Reactive rule already exists: " + id);
      reactive_rules.emplace(id, std::make_unique<rule>(id, name, content));
      return *reactive_rules[id];
    }
    rule &create_deliberative_rule(const std::string &id, const std::string &name, const std::string &content)
    {
      if (deliberative_rules.find(id) != deliberative_rules.end())
        throw std::invalid_argument("Deliberative rule already exists: " + id);
      deliberative_rules.emplace(id, std::make_unique<rule>(id, name, content));
      return *deliberative_rules[id];
    }

  private:
    const json::json config; // The app name.

    std::map<std::string, std::unique_ptr<type>> types_by_id;          // The types stored in the database by ID.
    std::map<std::string, std::reference_wrapper<type>> types_by_name; // The types stored in the database by name.
    std::map<std::string, std::unique_ptr<item>> items;                // The items stored in the database by ID.
    std::map<std::string, std::unique_ptr<rule>> reactive_rules;       // The reactive rules stored in the database by ID.
    std::map<std::string, std::unique_ptr<rule>> deliberative_rules;   // The deliberative rules stored in the database by ID.
  };
} // namespace coco
