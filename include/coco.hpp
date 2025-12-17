#pragma once

#include "json.hpp"
#include "clips.h"
#include <chrono>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <random>
#include <typeindex>

#ifdef BUILD_LISTENERS
#define CREATED_RULE(rr) created_rule(rr)
#else
#define CREATED_RULE(rr)
#endif

namespace coco
{
  class coco_db;
  class db_type;
  class coco_module;
  class type;
  class item;
  class property_type;
  class property;
  class rule;
#ifdef BUILD_LISTENERS
  class listener;
#endif

  class coco
  {
    friend class coco_module;
    friend class type;
    friend class item;
    friend class property_type;
    friend class property;
    friend class rule;
#ifdef BUILD_LISTENERS
    friend class listener;
#endif

  public:
    coco(coco_db &db) noexcept;
    ~coco();

    /**
     * @brief Initializes the CoCo environment by loading rules from the database.
     *
     * This function retrieves all rules stored in the database and loads them into the CoCo environment.
     */
    void load_rules() noexcept;

    [[nodiscard]] coco_db &get_db() noexcept { return db; }
    [[nodiscard]] const coco_db &get_db() const noexcept { return db; }

    template <typename Tp, typename... Args>
    Tp &add_module(Args &&...args)
    {
      static_assert(std::is_base_of<coco_module, Tp>::value, "Extension must be derived from coco_module");
      if (auto it = modules.find(typeid(Tp)); it == modules.end())
      {
        auto mod = std::make_unique<Tp>(std::forward<Args>(args)...);
        auto &ref = *mod;
        modules.emplace(typeid(Tp), std::move(mod));
        return ref;
      }
      else
        throw std::runtime_error("Module already exists");
    }

    template <typename Tp>
    [[nodiscard]] Tp &get_module() const
    {
      static_assert(std::is_base_of<coco_module, Tp>::value, "Extension must be derived from coco_module");
      if (auto it = modules.find(typeid(Tp)); it != modules.end())
        return *static_cast<Tp *>(it->second.get());
      throw std::runtime_error("Module not found");
    }

    /**
     * @brief Returns a vector of references to the types.
     *
     * This function retrieves all the types stored in the database and returns them as a vector of `type` objects. The returned vector contains references to the actual types stored in the `types` map.
     *
     * @return A vector of types.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<type>> get_types() noexcept;

    /**
     * @brief Retrieves a type with the specified name.
     *
     * This function retrieves the type with the specified name.
     *
     * @param name The name of the type.
     * @return A reference to the type.
     * @throws std::invalid_argument if the type does not exist.
     */
    [[nodiscard]] type &get_type(std::string_view name);
    [[nodiscard]] type &create_type(std::string_view name, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json(), bool infere = true) noexcept;
    void delete_type(type &tp, bool infere = true) noexcept;

    /**
     * @brief Returns a vector of references to the items.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of items.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<item>> get_items() noexcept;

    /**
     * @brief Retrieves all items of a specific type.
     *
     * This function retrieves all items that belong to the specified type.
     *
     * @param tp The type of the items to retrieve.
     * @return A vector of references to the items of the specified type.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<item>> get_items(const type &tp) noexcept;

    /**
     * @brief Retrieves an item with the specified ID.
     *
     * This function retrieves the item with the specified ID.
     *
     * @param id The ID of the item.
     * @return A reference to the item.
     * @throws std::invalid_argument if the item does not exist.
     */
    [[nodiscard]] item &get_item(std::string_view id);
    /**
     * @brief Creates a new item.
     *
     * This function creates a new item with the specified types, properties, and value.
     *
     * @param tps A vector of references to the types of the item.
     * @param props The properties of the item as a JSON object.
     * @param val The value of the item as an optional pair of JSON object and timestamp.
     * @param infere Whether to run inference after creating the item.
     * @return A reference to the newly created item.
     */
    [[nodiscard]] item &create_item(std::vector<std::reference_wrapper<type>> &&tps = {}, json::json &&props = json::json(), std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val = std::nullopt, bool infere = true) noexcept;
    /**
     * @brief Sets the properties of an item.
     *
     * This function sets the properties of the specified item using the provided JSON object.
     *
     * @param itm The item whose properties are to be set.
     * @param props The JSON object containing the properties to be set.
     * @param infere Whether to run inference after setting the properties.
     */
    void set_properties(item &itm, json::json &&props, bool infere = true) noexcept;
    /**
     * @brief Retrieves the values of an item within a specified time range.
     *
     * This function retrieves the values of the specified item that fall within the given time range.
     *
     * @param itm The item whose values are to be retrieved.
     * @param from The start time of the range.
     * @param to The end time of the range.
     * @return A JSON object containing the values of the item within the specified time range.
     */
    [[nodiscard]] json::json get_values(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to = std::chrono::system_clock::now());
    /**
     * @brief Sets the value of an item.
     *
     * This function sets the value of the specified item using the provided JSON object and timestamp.
     *
     * @param itm The item whose value is to be set.
     * @param val The JSON object representing the value to be set.
     * @param timestamp The timestamp associated with the value.
     * @param infere Whether to run inference after setting the value.
     */
    void set_value(item &itm, json::json &&val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now(), bool infere = true);
    /**
     * @brief Deletes an item.
     *
     * This function deletes the specified item from the CoCo environment.
     *
     * @param itm The item to be deleted.
     * @param infere Whether to run inference after deleting the item.
     */
    void delete_item(item &itm, bool infere = true) noexcept;

    /**
     * @brief Returns a vector of references to the rules.
     *
     * This function retrieves all the rules stored in the database and returns them as a vector of `rule` objects. The returned vector contains references to the actual rules stored in the `rules` map.
     *
     * @return A vector of rules.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<rule>> get_rules() noexcept;
    /**
     * @brief Retrieves a rule with the specified name.
     *
     * This function retrieves the rule with the specified name.
     *
     * @param name The name of the rule.
     * @return A reference to the rule.
     */
    [[nodiscard]] rule &get_rule(std::string_view name);
    /**
     * @brief Creates a new rule.
     *
     * This function creates a new rule with the specified name and content.
     *
     * @param rule_name The name of the rule.
     * @param rule_content The content of the rule.
     * @param infere Whether to run inference after creating the rule.
     * @return A reference to the newly created rule.
     */
    [[nodiscard]] rule &create_rule(std::string_view rule_name, std::string_view rule_content, bool infere = true);

    [[nodiscard]] json::json to_json() noexcept;

  protected:
    void add_property_type(std::unique_ptr<property_type> pt);

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    [[nodiscard]] property_type &get_property_type(std::string_view name) const;

    type &make_type(std::string_view name, json::json &&data = json::json());
    item &make_item(std::string_view id, std::vector<std::reference_wrapper<type>> &&tps, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val = std::nullopt);

    friend void add_type(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void remove_type(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void set_props(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void add_data(Environment *env, UDFContext *udfc, UDFValue *out);

    friend void add_types(coco &cc, std::vector<std::string> &&type_files) noexcept;
    friend void set_types(coco &cc, std::vector<db_type> &&db_types) noexcept;

#ifdef BUILD_LISTENERS
  private:
    /**
     * @brief Notifies when the type is created.
     *
     * @param tp The created type.
     */
    void created_type(const type &tp) const;

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    void created_item(const item &itm) const;

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    void updated_item(const item &itm) const;

    /**
     * @brief Notifies when new data is added to the item.
     *
     * @param itm The item.
     * @param data The data.
     * @param timestamp The timestamp of the data.
     */
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) const;

    /**
     * @brief Notifies when the item is deleted.
     *
     * @param itm The deleted item.
     */
    void created_rule(const rule &rr) const;
#endif

  protected:
    coco_db &db;                                                                       // The database..
    std::unordered_map<std::type_index, std::unique_ptr<coco_module>> modules;         // The modules..
    json::json schemas;                                                                // The JSON schemas..
    std::mt19937 gen;                                                                  // The random number generator..
    std::map<std::string, std::unique_ptr<property_type>, std::less<>> property_types; // The property types..
    std::recursive_mutex mtx;                                                          // The mutex for the core..
    Environment *env;                                                                  // The CLIPS environment..
    std::map<std::string, std::unique_ptr<type>, std::less<>> types;                   // The types managed by CoCo by name.
    std::unordered_map<std::string, std::unique_ptr<item>> items;                      // The items by their ID..
    std::map<std::string, std::unique_ptr<rule>, std::less<>> rules;                   // The rules..
#ifdef BUILD_LISTENERS
    std::vector<listener *> listeners; // The CoCo listeners..
#endif
  };

  void set_types(coco &cc, std::vector<db_type> &&db_types) noexcept;

  void add_type(Environment *env, UDFContext *udfc, UDFValue *out);
  void remove_type(Environment *env, UDFContext *udfc, UDFValue *out);
  void set_props(Environment *env, UDFContext *udfc, UDFValue *out);
  void add_data(Environment *env, UDFContext *udfc, UDFValue *out);

  void empty_agenda(Environment *env, UDFContext *udfc, UDFValue *out);
  void multifield_to_json(Environment *env, UDFContext *udfc, UDFValue *out);
  void json_to_multifield(Environment *env, UDFContext *udfc, UDFValue *out);

#ifdef BUILD_LISTENERS
  class listener
  {
    friend class coco;

  public:
    listener(coco &cc) noexcept;
    virtual ~listener();

  private:
    /**
     * @brief Notifies when the type is created.
     *
     * @param tp The created type.
     */
    virtual void created_type([[maybe_unused]] const type &tp) {}

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    virtual void created_item([[maybe_unused]] const item &itm) {}

    /**
     * @brief Notifies when the item is updated.
     *
     * @param itm The updated item.
     */
    virtual void updated_item([[maybe_unused]] const item &itm) {}

    /**
     * @brief Notifies when new data is added to the item.
     *
     * @param itm The item.
     * @param data The data.
     * @param timestamp The timestamp of the data.
     */
    virtual void new_data([[maybe_unused]] const item &itm, [[maybe_unused]] const json::json &data, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp) {}

    /**
     * @brief Notifies when the rule is created.
     *
     * @param rr The created rule.
     */
    virtual void created_rule([[maybe_unused]] const rule &rr) {}

  private:
    coco &cc;
  };
#endif
} // namespace coco