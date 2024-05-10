#pragma once

#include "coco_type.hpp"
#include "coco_item.hpp"

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
     * @param pars The parameters of the type.
     * @return A reference to the created type.
     */
    virtual type &create_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars) = 0;

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
     * @param data Optional data for the item (default is an empty JSON object).
     * @return A reference to the created item.
     */
    virtual item &create_item(const type &type, const std::string &name, json::json &&data = {}) = 0;

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

  protected:
    type &create_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars)
    {
      if (types.find(id) != types.end())
        throw std::invalid_argument("Type already exists: " + id);
      types[id] = std::make_unique<type>(id, name, description, std::move(pars));
      return *types[id];
    }
    item &create_item(const std::string &id, const type &type, const std::string &name, json::json &&data = {})
    {
      if (items.find(id) != items.end())
        throw std::invalid_argument("item already exists: " + id);
      items[id] = std::make_unique<item>(id, type, name, std::move(data));
      return *items[id];
    }

  private:
    const json::json config; // The app name.

    std::map<std::string, std::unique_ptr<type>> types;
    std::map<std::string, std::unique_ptr<item>> items;
  };
} // namespace coco
