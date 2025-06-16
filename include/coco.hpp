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
#define CREATED_REACTIVE_RULE(rr) created_reactive_rule(rr)
#else
#define CREATED_REACTIVE_RULE(rr)
#endif

namespace coco
{
  constexpr const char *type_deftemplate = "(deftemplate type (slot name (type SYMBOL)))";
  constexpr const char *is_a_deftemplate = "(deftemplate is_a (slot type (type SYMBOL)) (slot parent (type SYMBOL)))";
  constexpr const char *item_deftemplate = "(deftemplate item (slot id (type SYMBOL)))";
  constexpr const char *instance_of_deftemplate = "(deftemplate instance_of (slot id (type SYMBOL)) (slot type (type SYMBOL)))";
  constexpr const char *inheritance_rule = "(defrule inheritance (is_a (type ?t) (parent ?p)) (instance_of (id ?i) (type ?t)) => (assert (instance_of (id ?i) (type ?p))))";
  constexpr const char *all_instances_of_function = "(deffunction all-instances-of (?type) (bind ?instances (create$)) (do-for-all-facts ((?instance_of instance_of)) (eq ?instance_of:type ?type) (bind ?instances (create$ ?instances ?instance_of:id))) (return ?instances))";

  class coco_db;
  class coco_module;
  class type;
  class item;
  class property_type;
  class property;
  class reactive_rule;
#ifdef BUILD_LISTENERS
  class listener;
#endif

  class coco
  {
    friend class coco_module;
    friend class type;
    friend class item;
    friend class property;
    friend class reactive_rule;
#ifdef BUILD_LISTENERS
    friend class listener;
#endif

  public:
    coco(coco_db &db) noexcept;
    ~coco();

    [[nodiscard]] coco_db &get_db() noexcept { return db; }

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
    [[nodiscard]] type &create_type(std::string_view name, std::vector<std::reference_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json(), bool infere = true) noexcept;
    void set_parents(type &tp, std::vector<std::reference_wrapper<const type>> &&parents, bool infere = true) noexcept;
    void delete_type(type &tp, bool infere = true) noexcept;

    /**
     * @brief Returns a vector of references to the items.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of items.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<item>> get_items() noexcept;

    [[nodiscard]] item &get_item(std::string_view id);
    [[nodiscard]] item &create_item(type &tp, json::json &&props = json::json(), std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val = std::nullopt, bool infere = true) noexcept;
    void set_properties(item &itm, json::json &&props, bool infere = true) noexcept;
    [[nodiscard]] json::json get_values(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to = std::chrono::system_clock::now());
    void set_value(item &itm, json::json &&val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now(), bool infere = true);
    void delete_item(item &itm, bool infere = true) noexcept;

    [[nodiscard]] std::vector<std::reference_wrapper<reactive_rule>> get_reactive_rules() noexcept;
    void create_reactive_rule(std::string_view rule_name, std::string_view rule_content, bool infere = true);

    [[nodiscard]] json::json to_json() noexcept;

  protected:
    void add_property_type(std::unique_ptr<property_type> pt);

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    [[nodiscard]] property_type &get_property_type(std::string_view name) const;

    type &make_type(std::string_view name, std::vector<std::reference_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json());

    friend void set_props(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void add_data(Environment *env, UDFContext *udfc, UDFValue *out);

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
    void created_reactive_rule(const reactive_rule &rr) const;
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
    std::map<std::string, std::unique_ptr<reactive_rule>, std::less<>> reactive_rules; // The reactive rules..
#ifdef BUILD_LISTENERS
    std::vector<listener *> listeners; // The CoCo listeners..
#endif
  };

  /**
   * @brief Represents a reactive CoCo rule.
   *
   * This class represents a reactive CoCo rule in the form of a name and content.
   */
  class reactive_rule final
  {
  public:
    /**
     * @brief Constructs a rule object.
     *
     * @param cc The CoCo core object.
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    reactive_rule(coco &cc, std::string_view name, std::string_view content) noexcept;
    ~reactive_rule();

    /**
     * @brief Gets the name of the rule.
     *
     * @return The name of the rule.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the content of the rule.
     *
     * @return The content of the rule.
     */
    [[nodiscard]] const std::string &get_content() const { return content; }

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    coco &cc;            // the CoCo core object.
    std::string name;    // the name of the rule.
    std::string content; // the content of the rule.
  };

  void set_props(Environment *env, UDFContext *udfc, UDFValue *out);
  void add_data(Environment *env, UDFContext *udfc, UDFValue *out);
  void multifield_to_json(Environment *env, UDFContext *udfc, UDFValue *out);

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
     * @brief Notifies when the reactive rule is created.
     *
     * @param rr The created reactive rule.
     */
    virtual void created_reactive_rule([[maybe_unused]] const reactive_rule &rr) {}

  private:
    coco &cc;
  };
#endif
} // namespace coco