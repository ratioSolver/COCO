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
     * @brief Creates a parameter with the given name, description, and schema.
     *
     * This function is virtual and must be implemented by derived classes.
     *
     * @param name The name of the parameter.
     * @param description The description of the parameter.
     * @param schema The JSON schema for the parameter.
     * @return A reference to the created parameter.
     */
    virtual parameter &create_parameter(const std::string &name, const std::string &description, json::json &&schema) = 0;

    /**
     * Sets the name of the given parameter.
     *
     * @param par The parameter to set the name for.
     * @param name The name to set for the parameter.
     */
    virtual void set_parameter_name(parameter &par, const std::string &name) { par.name = name; }

    /**
     * @brief Sets the description of a parameter.
     *
     * This function sets the description of a given parameter. The description provides additional information about the parameter.
     *
     * @param par The parameter to set the description for.
     * @param description The description to set for the parameter.
     */
    virtual void set_parameter_description(parameter &par, const std::string &description) { par.description = description; }

    /**
     * @brief Adds a super parameter to the given parameter.
     *
     * This function adds a super parameter to the given parameter. A super parameter is a parameter that the given parameter extends.
     *
     * @param par The parameter to which the super parameter will be added.
     * @param super_par The super parameter to add.
     */
    virtual void add_super_parameter(parameter &par, const parameter &super_par) { par.super_parameters.push_back(super_par); }

    /**
     * @brief Removes a super parameter from the given parameter.
     *
     * This function removes the super parameter with the specified ID from the given parameter.
     *
     * @param par The parameter from which to remove the super parameter.
     * @param super_par The super parameter to remove.
     */
    virtual void remove_super_parameter(parameter &par, const parameter &super_par)
    {
      par.super_parameters.erase(std::find_if(par.super_parameters.begin(), par.super_parameters.end(), [&super_par](const parameter &p)
                                              { return p.id == super_par.id; }));
    }

    /**
     * @brief Adds a disjoint parameter to the given parameter.
     *
     * This function adds a disjoint parameter to the given parameter. A disjoint parameter is a parameter that is disjoint with the given parameter.
     *
     * @param par The parameter to which the disjoint parameter will be added.
     * @param disjoint_par The disjoint parameter to add.
     */
    virtual void add_disjoint_parameter(parameter &par, const parameter &disjoint_par) { par.disjoint_parameters.push_back(disjoint_par); }

    /**
     * @brief Removes a disjoint parameter from the given parameter.
     *
     * This function removes the disjoint parameter with the specified ID from the given parameter.
     *
     * @param par The parameter from which to remove the disjoint parameter.
     * @param disjoint_par The disjoint parameter to remove.
     */
    virtual void remove_disjoint_parameter(parameter &par, const parameter &disjoint_par)
    {
      par.disjoint_parameters.erase(std::find_if(par.disjoint_parameters.begin(), par.disjoint_parameters.end(), [&disjoint_par](const parameter &p)
                                                 { return p.id == disjoint_par.id; }));
    }

    /**
     * @brief Sets the schema of a parameter.
     *
     * This function sets the schema of a given parameter. The schema defines the structure of the parameter.
     *
     * @param par The parameter to set the schema for.
     * @param schema The schema to set for the parameter.
     */
    virtual void set_parameter_schema(parameter &par, json::json &&schema) { par.schema = schema; }

    /**
     * @brief Removes a parameter from the database.
     *
     * This function removes the specified parameter from the database.
     *
     * @param par The parameter to be removed.
     */
    virtual void delete_parameter(const parameter &par) { parameters.erase(par.id); }

    /**
     * Retrieves a vector of references to the parameters in the database.
     *
     * This function retrieves all the parameters stored in the database and returns them as a vector of `parameter` objects. The returned vector contains references to the actual parameters stored in the `parameters` map.
     *
     * @return A vector of references to the parameters.
     */
    std::vector<std::reference_wrapper<parameter>> get_parameters() const
    {
      std::vector<std::reference_wrapper<parameter>> res;
      for (auto &p : parameters)
        res.push_back(*p.second);
      return res;
    }

    /**
     * Retrieves the reference to a parameter based on its ID.
     *
     * @param id The ID of the parameter to retrieve.
     * @return A reference to the parameter with the specified ID.
     * @throws std::invalid_argument if the parameter with the specified ID is not found.
     */
    parameter &get_parameter(const std::string &id) const
    {
      if (parameters.find(id) == parameters.end())
        throw std::invalid_argument("Parameter not found: " + id);
      return *parameters.at(id);
    }

    /**
     * Retrieves the reference to a parameter based on its name.
     *
     * @param name The name of the parameter to retrieve.
     * @return A reference to the parameter with the specified name.
     * @throws std::invalid_argument if the parameter with the specified name is not found.
     */
    parameter &get_parameter_by_name(const std::string &name) const
    {
      if (parameters_by_name.find(name) == parameters_by_name.end())
        throw std::invalid_argument("Parameter not found: " + name);
      return parameters_by_name.at(name);
    }

    /**
     * @brief Creates a new type.
     *
     * This function creates a new type with the specified name, description, and parameters.
     *
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_pars The static parameters of the type.
     * @param dynamic_pars The dynamic parameters of the type.
     * @return A reference to the created type.
     */
    virtual type &create_type(const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const parameter>> &&static_pars, std::vector<std::reference_wrapper<const parameter>> &&dynamic_pars) = 0;

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
     * @brief Adds a super type to the given type.
     *
     * This function adds a super type to the given type. A super type is a type that the given type extends.
     *
     * @param tp The type to which the super type will be added.
     * @param st The super type to add.
     */
    virtual void add_super_type(type &tp, const type &st) { tp.super_types.push_back(st); }

    /**
     * @brief Removes a super type from the given type.
     *
     * This function removes the super type with the specified ID from the given type.
     *
     * @param tp The type from which to remove the super type.
     * @param st The ID of the super type to remove.
     */
    virtual void remove_super_type(type &tp, const type &st)
    {
      tp.super_types.erase(std::find_if(tp.super_types.begin(), tp.super_types.end(), [&st](const type &t)
                                        { return t.id == st.id; }));
    }

    /**
     * @brief Adds a disjoint type to the given type.
     *
     * This function adds a disjoint type to the given type. A disjoint type is a type that is disjoint with the given type.
     *
     * @param tp The type to which the disjoint type will be added.
     * @param disjoint_type The disjoint type to add.
     */
    virtual void add_disjoint_type(type &tp, const type &disjoint_type) { tp.disjoint_types.push_back(disjoint_type); }

    /**
     * @brief Removes a disjoint type from the given type.
     *
     * This function removes the disjoint type with the specified ID from the given type.
     *
     * @param tp The type from which to remove the disjoint type.
     * @param disjoint_type The disjoint type to remove.
     */
    virtual void remove_disjoint_type(type &tp, const type &disjoint_type)
    {
      tp.disjoint_types.erase(std::find_if(tp.disjoint_types.begin(), tp.disjoint_types.end(), [&disjoint_type](const type &t)
                                           { return t.id == disjoint_type.id; }));
    }

    /**
     * Adds a static parameter to the given type.
     *
     * @param tp The type to which the static parameter will be added.
     * @param par The static parameter to add.
     */
    virtual void add_static_parameter(type &tp, const parameter &par) { tp.static_parameters.push_back(par); }

    /**
     * @brief Removes a static parameter from the given type.
     *
     * This function removes the static parameter with the specified id from the given type.
     *
     * @param tp The type from which to remove the static parameter.
     * @param par The static parameter to remove.
     */
    virtual void remove_static_parameter(type &tp, const parameter &par)
    {
      tp.static_parameters.erase(std::find_if(tp.static_parameters.begin(), tp.static_parameters.end(), [&par](const parameter &p)
                                              { return p.id == par.id; }));
    }

    /**
     * Adds a dynamic parameter to the given type.
     *
     * @param tp The type to which the dynamic parameter will be added.
     * @param par The dynamic parameter to add.
     */
    virtual void add_dynamic_parameter(type &tp, const parameter &par) { tp.dynamic_parameters.push_back(par); }

    /**
     * @brief Removes a dynamic parameter from the given type.
     *
     * This function removes the dynamic parameter with the specified id from the given type.
     *
     * @param tp The type from which to remove the dynamic parameter.
     * @param par The dynamic parameter to remove.
     */
    virtual void remove_dynamic_parameter(type &tp, const parameter &par)
    {
      tp.dynamic_parameters.erase(std::find_if(tp.dynamic_parameters.begin(), tp.dynamic_parameters.end(), [&par](const parameter &p)
                                               { return p.id == par.id; }));
    }

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
     * @param pars The parameters of the item.
     * @return A reference to the created item.
     */
    virtual item &create_item(const type &tp, const std::string &name, const json::json &pars) = 0;

    /**
     * Sets the name of an item.
     *
     * @param item The item to set the name for.
     * @param name The new name for the item.
     */
    virtual void set_item_name(item &item, const std::string &name) { item.name = name; }
    /**
     * @brief Sets the parameters of an item.
     *
     * This function sets the parameters of the given item using the provided JSON object.
     *
     * @param item The item to set the parameters for.
     * @param pars The JSON object containing the parameters.
     */
    virtual void set_item_parameters(item &item, const json::json &pars) { item.parameters = pars; }

    /**
     * @brief Removes an item from the database.
     *
     * This function removes the specified item from the database.
     *
     * @param item The item to be removed.
     */
    virtual void delete_item(const item &item) { items.erase(item.id); }

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
     * @param item The item to which the data will be added.
     * @param timestamp The timestamp of the data.
     * @param data The data to be added to the database.
     */
    virtual void add_data(const item &item, const std::chrono::system_clock::time_point &timestamp, const json::json &data) = 0;

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
    parameter &create_parameter(const std::string &id, const std::string &name, const std::string &description, json::json &&schema)
    {
      if (parameters.find(id) != parameters.end())
        throw std::invalid_argument("Parameter already exists: " + id);
      auto par = std::make_unique<parameter>(id, name, description, std::move(schema));
      parameters_by_name.emplace(name, *par);
      parameters.emplace(id, std::move(par));
      return *parameters[id];
    }
    type &create_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const parameter>> &&static_pars, std::vector<std::reference_wrapper<const parameter>> &&dynamic_pars)
    {
      if (types_by_id.find(id) != types_by_id.end())
        throw std::invalid_argument("Type already exists: " + id);
      auto tp = std::make_unique<type>(id, name, description, std::move(static_pars), std::move(dynamic_pars));
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

    std::map<std::string, std::unique_ptr<parameter>> parameters;                // The parameters stored in the database by ID.
    std::map<std::string, std::reference_wrapper<parameter>> parameters_by_name; // The parameters stored in the database by name.
    std::map<std::string, std::unique_ptr<type>> types_by_id;                    // The types stored in the database by ID.
    std::map<std::string, std::reference_wrapper<type>> types_by_name;           // The types stored in the database by name.
    std::map<std::string, std::unique_ptr<item>> items;                          // The items stored in the database by ID.
    std::map<std::string, std::unique_ptr<rule>> reactive_rules;                 // The reactive rules stored in the database by ID.
    std::map<std::string, std::unique_ptr<rule>> deliberative_rules;             // The deliberative rules stored in the database by ID.
  };
} // namespace coco
