#pragma once

#include "coco_type.hpp"
#include "coco_item.hpp"
#include <chrono>

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
     * This function creates a new type with the specified name, description, and parameters.
     *
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_pars The static parameters of the type.
     * @param dynamic_pars The dynamic parameters of the type.
     * @return A reference to the created type.
     */
    virtual type &create_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&static_pars, std::vector<std::unique_ptr<parameter>> &&dynamic_pars) = 0;

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
     * @brief Creates an item of the specified type with the given name and optional data.
     *
     * @param type The type of the item.
     * @param name The name of the item.
     * @param pars The parameters of the item.
     * @return A reference to the created item.
     */
    virtual item &create_item(const type &type, const std::string &name, const json::json &pars) = 0;

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
     * @brief Adds data to the database for a given item.
     *
     * This function adds the provided data to the database for the specified item.
     *
     * @param item The item to which the data will be added.
     * @param timestamp The timestamp of the data.
     * @param data The data to be added to the database.
     */
    virtual void add_data(const item &item, const std::chrono::system_clock::time_point &timestamp, const json::json &data) = 0;

  protected:
    type &create_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&static_pars, std::vector<std::unique_ptr<parameter>> &&dynamic_pars)
    {
      if (types.find(id) != types.end())
        throw std::invalid_argument("Type already exists: " + id);
      types[id] = std::make_unique<type>(id, name, description, std::move(static_pars), std::move(dynamic_pars));
      return *types[id];
    }
    item &create_item(const std::string &id, const type &type, const std::string &name, const json::json &pars)
    {
      if (items.find(id) != items.end())
        throw std::invalid_argument("item already exists: " + id);
      items[id] = std::make_unique<item>(id, type, name, pars);
      return *items[id];
    }

  private:
    const json::json config; // The app name.

    std::map<std::string, std::unique_ptr<type>> types;
    std::map<std::string, std::unique_ptr<item>> items;
  };
} // namespace coco
