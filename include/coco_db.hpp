#pragma once

#include "coco_type.hpp"
#include "coco_item.hpp"
#include "coco_rule.hpp"
#include <chrono>
#include <stdexcept>

namespace coco
{
  class coco_core;

  class coco_db
  {
  public:
    coco_db(const json::json &config = {});
    virtual ~coco_db() = default;

    const json::json &get_config() const { return config; }

    /**
     * Initializes the coco_db object.
     *
     * @param cc The coco_core object used for initialization.
     */
    virtual void init(coco_core &cc);

#ifdef ENABLE_AUTH
    /**
     * @brief Creates a new user.
     *
     * This function creates a new user with the specified username, password, personal data, and data.
     *
     * @param cc The CoCo core object.
     * @param username The username of the user.
     * @param password The password of the user.
     * @param personal_data The personal data associated with the user.
     * @param data The data associated with the user.
     * @return The created user object.
     */
    virtual item &create_user(coco_core &cc, const std::string &username, const std::string &password, json::json &&personal_data = {}, json::json &&data = {}) = 0;

    /**
     * @brief Retrieves a user item based on the provided username and password.
     *
     * This function attempts to find and return a reference to a user item that matches
     * the given username and password. If no matching user is found, it returns an empty
     * optional.
     *
     * @param username The username of the user to retrieve.
     * @param password The password of the user to retrieve.
     * @return std::optional<std::reference_wrapper<item>> An optional containing a reference
     *         to the user item if found, otherwise an empty optional.
     */
    virtual std::optional<std::reference_wrapper<item>> get_user(const std::string &username, const std::string &password) = 0;

    /**
     * @brief Sets the username of the given user.
     *
     * This function sets the username of the given user object to the specified username.
     *
     * @param usr The user object to set the username for.
     * @param username The new username for the user.
     */
    virtual void set_user_username(item &usr, const std::string &username) = 0;

    /**
     * @brief Sets the password of the given user.
     *
     * This function sets the password of the given user object to the specified password.
     *
     * @param usr The user object to set the password for.
     * @param password The new password for the user.
     */
    virtual void set_user_password(item &usr, const std::string &password) = 0;

    /**
     * @brief Sets the personal data of the given user.
     *
     * This function sets the personal data of the given user object to the specified personal data.
     *
     * @param usr The user object to set the personal data for.
     * @param personal_data The new personal data for the user.
     */
    virtual void set_user_personal_data(item &usr, json::json &&personal_data) = 0;

    /**
     * @brief Deletes the specified user.
     *
     * This function deletes the specified user from the database.
     *
     * @param usr The user to be deleted.
     */
    virtual void delete_user(const item &usr) = 0;
#endif

    /**
     * @brief Creates a new type.
     *
     * This function creates a new type with the specified name, description, and properties.
     *
     * @param cc The CoCo core object.
     * @param name The name of the type.
     * @param description The description of the type.
     * @param props The properties of the type.
     * @param parents The parent types of the type.
     * @param static_properties The static properties of the type.
     * @param dynamic_properties The dynamic properties of the type.
     * @return A reference to the created type.
     */
    virtual type &create_type(coco_core &cc, const std::string &name, const std::string &description, json::json &&props) = 0;

    /**
     * Sets the name of the given type.
     *
     * @param tp The type to set the name for.
     * @param name The name to set for the type.
     */
    virtual void set_type_name(type &tp, const std::string &name) { tp.set_name(name); }

    /**
     * @brief Sets the description of a type.
     *
     * This function sets the description of a given type. The description provides additional information about the type.
     *
     * @param tp The type to set the description for.
     * @param description The description to set for the type.
     */
    virtual void set_type_description(type &tp, const std::string &description) { tp.set_description(description); }

    /**
     * @brief Sets the properties of the given type object.
     *
     * This function sets the properties of the provided type object using the
     * specified JSON properties.
     *
     * @param tp A reference to the type object whose properties are to be set.
     * @param props A constant reference to a JSON object containing the properties
     * to be set on the type object.
     */
    virtual void set_type_properties(type &tp, json::json &&props) { tp.set_properties(std::move(props)); }

    /**
     * @brief Sets the parents of the given type object.
     *
     * This function assigns a list of parent objects to the specified type object.
     *
     * @param tp The type object whose parents are to be set.
     * @param parents A vector of references to the parent objects.
     */
    virtual void set_type_parents(type &tp, std::vector<std::reference_wrapper<const type>> &&parents) { tp.set_parents(parents); }

    /**
     * @brief Sets the static properties of a given type.
     *
     * This function assigns a set of static properties to the specified type.
     *
     * @param tp The type to which the static properties will be assigned.
     * @param props A vector of unique pointers to property objects that will be set as static properties for the type.
     */
    virtual void set_type_static_properties(type &tp, std::vector<std::unique_ptr<property>> &&props) { tp.set_static_properties(std::move(props)); }

    /**
     * @brief Sets the dynamic properties of a given type.
     *
     * This function assigns a set of dynamic properties to the specified type.
     * It takes ownership of the properties by moving them.
     *
     * @param tp The type to which the dynamic properties will be assigned.
     * @param props A vector of unique pointers to properties that will be set as dynamic properties of the type.
     */
    virtual void set_type_dynamic_properties(type &tp, std::vector<std::unique_ptr<property>> &&props) { tp.set_dynamic_properties(std::move(props)); }

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
      types.erase(tp.id);
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
      for (auto &s : types)
        res.push_back(*s.second);
      return res;
    }

    /**
     * @brief Checks if a type with the given identifier exists.
     *
     * This function searches for the specified type identifier in the types collection.
     *
     * @param id The identifier of the type to check.
     * @return true if the type exists, false otherwise.
     */
    bool has_type(const std::string &id) const { return types.find(id) != types.end(); }

    /**
     * Retrieves the reference to a type based on its ID.
     *
     * @param id The ID of the type to retrieve.
     * @return A reference to the type with the specified ID.
     * @throws std::invalid_argument if the type with the specified ID is not found.
     */
    type &get_type(const std::string &id) const
    {
      if (types.find(id) == types.end())
        throw std::invalid_argument("Type not found: " + id);
      return *types.at(id);
    }

    /**
     * @brief Checks if a type with the given name exists in the collection.
     *
     * @param name The name of the type to check for.
     * @return true if the type with the specified name exists, false otherwise.
     */
    bool has_type_name(const std::string &name) const { return types_by_name.find(name) != types_by_name.end(); }

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
     * @param cc The CoCo core object.
     * @param tp The type of the item.
     * @param props The properties of the item.
     * @param val The value of the item.
     * @param timestamp The timestamp of the value.
     * @return A reference to the created item.
     */
    virtual item &create_item(coco_core &cc, const type &tp, json::json &&props, const json::json &val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) = 0;

    /**
     * @brief Sets the properties of an item.
     *
     * This function sets the properties of the given item using the provided JSON object.
     *
     * @param itm The item to set the properties for.
     * @param props The JSON object containing the properties.
     */
    virtual void set_item_properties(item &itm, json::json &&props) { itm.set_properties(std::move(props)); }

    /**
     * @brief Sets the value of an item.
     *
     * This function sets the value of the given item using the provided JSON value.
     *
     * @param itm The item to set the value for.
     * @param value The JSON value to set as the item's value.
     * @param timestamp The timestamp of the value. Defaults to the current time.
     */
    virtual void set_item_value(item &itm, const json::json &value, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) { itm.set_value(value, timestamp); }

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
     * @brief Checks if an item with the given identifier exists.
     *
     * This function searches for the specified item identifier in the items collection.
     *
     * @param id The identifier of the item to check.
     * @return true if the item exists, false otherwise.
     */
    bool has_item(const std::string &id) const { return items.find(id) != items.end(); }

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
     * Retrieves data for a given item within a specified time range.
     *
     * @param itm The item for which to retrieve data.
     * @param from The starting time of the data range.
     * @param to The ending time of the data range.
     * @return A JSON object containing the retrieved data.
     */
    virtual json::json get_data(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to) = 0;

    /**
     * @brief Adds data to the database for a given item.
     *
     * This function adds the provided data to the database for the specified item.
     *
     * @param itm The item to which the data will be added.
     * @param data The data to be added to the database.
     * @param timestamp The timestamp of the data. Defaults to the current time.
     */
    virtual void add_data(item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) = 0;

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
     * Retrieves the reactive rule with the specified ID.
     *
     * @param id The ID of the reactive rule to retrieve.
     * @return A reference to the reactive rule.
     * @throws std::invalid_argument if the reactive rule with the specified ID is not found.
     */
    rule &get_reactive_rule(const std::string &id) const
    {
      if (reactive_rules.find(id) == reactive_rules.end())
        throw std::invalid_argument("Reactive rule not found: " + id);
      return *reactive_rules.at(id);
    }

    /**
     * Retrieves a reactive rule by its name.
     *
     * @param name The name of the reactive rule to retrieve.
     * @return A reference to the reactive rule.
     * @throws std::invalid_argument if the reactive rule with the given name is not found.
     */
    rule &get_reactive_rule_by_name(const std::string &name) const
    {
      if (reactive_rules_by_name.find(name) == reactive_rules_by_name.end())
        throw std::invalid_argument("Reactive rule not found: " + name);
      return reactive_rules_by_name.at(name);
    }

    /**
     * @brief Creates a reactive rule with the given name and content.
     *
     * @param cc The CoCo core object.
     * @param name The name of the reactive rule.
     * @param content The content of the reactive rule.
     * @return A reference to the created reactive rule.
     */
    virtual rule &create_reactive_rule(coco_core &cc, const std::string &name, const std::string &content) = 0;

    /**
     * @brief Sets the name of the reactive rule.
     *
     * This function is used to set the name of a reactive rule. The name is a string that helps identify the rule.
     *
     * @param r The reference to the rule object.
     * @param name The name of the reactive rule.
     */
    virtual void set_reactive_rule_name(rule &r, const std::string &name) { r.name = name; }

    /**
     * @brief Sets the content of a reactive rule.
     *
     * This function sets the content of the given reactive rule `r` to the specified `content`.
     *
     * @param r The reactive rule to set the content for.
     * @param content The content to set for the reactive rule.
     */
    virtual void set_reactive_rule_content(rule &r, const std::string &content) { r.content = content; }

    /**
     * @brief Deletes a reactive rule.
     *
     * This function deletes the specified reactive rule from the database.
     *
     * @param r The reactive rule to be deleted.
     */
    virtual void delete_reactive_rule(const rule &r) { reactive_rules.erase(r.id); }

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
     * Retrieves the deliberative rule with the specified ID.
     *
     * @param id The ID of the deliberative rule to retrieve.
     * @return A reference to the deliberative rule.
     * @throws std::invalid_argument if the deliberative rule with the specified ID is not found.
     */
    rule &get_deliberative_rule(const std::string &id) const
    {
      if (deliberative_rules.find(id) == deliberative_rules.end())
        throw std::invalid_argument("Deliberative rule not found: " + id);
      return *deliberative_rules.at(id);
    }

    /**
     * Retrieves a deliberative rule by its name.
     *
     * @param name The name of the deliberative rule to retrieve.
     * @return A reference to the deliberative rule.
     * @throws std::invalid_argument if the deliberative rule with the specified name is not found.
     */
    rule &get_deliberative_rule_by_name(const std::string &name) const
    {
      if (deliberative_rules_by_name.find(name) == deliberative_rules_by_name.end())
        throw std::invalid_argument("Deliberative rule not found: " + name);
      return deliberative_rules_by_name.at(name);
    }

    /**
     * @brief Creates a deliberative rule with the given name and content.
     *
     * @param cc The CoCo core object.
     * @param name The name of the deliberative rule.
     * @param content The content of the deliberative rule.
     * @return A reference to the created deliberative rule.
     */
    virtual rule &create_deliberative_rule(coco_core &cc, const std::string &name, const std::string &content) = 0;

    /**
     * @brief Sets the name of a deliberative rule.
     *
     * This function sets the name of the given deliberative rule to the specified name.
     *
     * @param r The deliberative rule to set the name for.
     * @param name The new name for the deliberative rule.
     */
    virtual void set_deliberative_rule_name(rule &r, const std::string &name) { r.name = name; }

    /**
     * @brief Sets the content of a deliberative rule.
     *
     * This function sets the content of the given deliberative rule to the specified content.
     *
     * @param r The rule to set the content for.
     * @param content The content to set for the rule.
     */
    virtual void set_deliberative_rule_content(rule &r, const std::string &content) { r.content = content; }

    /**
     * @brief Deletes a deliberative rule.
     *
     * This function is used to delete a deliberative rule from the database.
     *
     * @param r The rule to be deleted.
     */
    virtual void delete_deliberative_rule(const rule &r) { deliberative_rules.erase(r.id); }

  protected:
    type &create_type(coco_core &cc, const std::string &id, const std::string &name, const std::string &description, json::json &&props)
    {
      if (types.find(id) != types.end())
        throw std::invalid_argument("Type already exists: " + id);
      auto tp = std::make_unique<type>(cc, id, name, description, std::move(props));
      types_by_name.emplace(name, *tp);
      types.emplace(id, std::move(tp));
      return *types[id];
    }
    item &create_item(coco_core &cc, const std::string &id, const type &tp, json::json &&props, const json::json &val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now())
    {
      if (items.find(id) != items.end())
        throw std::invalid_argument("item already exists: " + id);
      items.emplace(id, std::make_unique<item>(cc, id, tp, std::move(props), val, timestamp));
      return *items[id];
    }
    rule &create_reactive_rule(coco_core &cc, const std::string &id, const std::string &name, const std::string &content)
    {
      if (reactive_rules.find(id) != reactive_rules.end())
        throw std::invalid_argument("Reactive rule already exists: " + id);
      auto rr = std::make_unique<rule>(cc, id, name, content);
      reactive_rules_by_name.emplace(name, *rr);
      reactive_rules.emplace(id, std::move(rr));
      return *reactive_rules[id];
    }
    rule &create_deliberative_rule(coco_core &cc, const std::string &id, const std::string &name, const std::string &content)
    {
      if (deliberative_rules.find(id) != deliberative_rules.end())
        throw std::invalid_argument("Deliberative rule already exists: " + id);
      auto dr = std::make_unique<rule>(cc, id, name, content);
      deliberative_rules_by_name.emplace(name, *dr);
      deliberative_rules.emplace(id, std::move(dr));
      return *deliberative_rules[id];
    }

  private:
    const json::json config; // The app name.

    std::map<std::string, std::unique_ptr<type>> types;                             // The types stored in the database by ID.
    std::map<std::string, std::reference_wrapper<type>> types_by_name;              // The types stored in the database by name.
    std::map<std::string, std::unique_ptr<item>> items;                             // The items stored in the database by ID.
    std::map<std::string, std::unique_ptr<rule>> reactive_rules;                    // The reactive rules stored in the database by ID.
    std::map<std::string, std::reference_wrapper<rule>> reactive_rules_by_name;     // The reactive rules stored in the database by name.
    std::map<std::string, std::unique_ptr<rule>> deliberative_rules;                // The deliberative rules stored in the database by ID.
    std::map<std::string, std::reference_wrapper<rule>> deliberative_rules_by_name; // The deliberative rules stored in the database by name.
  };
} // namespace coco
