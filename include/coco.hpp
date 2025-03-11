#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#include <chrono>
#include <optional>
#include <mutex>

namespace coco
{
  constexpr const char *type_deftemplate = "(deftemplate type (slot name (type SYMBOL)))";
  constexpr const char *is_a_deftemplate = "(deftemplate is_a (slot type (type SYMBOL)) (slot parent (type SYMBOL)))";
  constexpr const char *item_deftemplate = "(deftemplate item (slot id (type SYMBOL)))";
  constexpr const char *instance_of_deftemplate = "(deftemplate instance_of (slot id (type SYMBOL)) (slot type (type SYMBOL)))";
  constexpr const char *inheritance_rule = "(defrule inheritance (is_a (type ?t) (parent ?p)) (instance_of (id ?i) (type ?t)) => (assert (instance_of (id ?i) (type ?p))))";
  constexpr const char *all_instances_of_function = "(deffunction all-instances-of (?type) (bind ?instances (create$)) (do-for-all-facts ((?instance_of instance_of)) (eq ?instance_of:type ?type) (bind ?instances (create$ ?instances ?instance_of:id))) (return ?instances))";

  class coco_db;
  class type;
  class item;
  class property_type;
  class property;
  class reactive_rule;
  class deliberative_rule;
#ifdef BUILD_LISTENERS
  class listener;
#endif

  class coco
  {
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

    /**
     * @brief Returns a vector of references to the types.
     *
     * This function retrieves all the types stored in the database and returns them as a vector of `type` objects. The returned vector contains references to the actual types stored in the `types` map.
     *
     * @return A vector of types.
     */
    [[nodiscard]] std::vector<utils::ref_wrapper<type>> get_types() noexcept;

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

    [[nodiscard]] type &create_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json()) noexcept;
    void set_parents(type &tp, std::vector<utils::ref_wrapper<const type>> &&parents) noexcept;
    void delete_type(type &tp) noexcept;

    [[nodiscard]] item &create_item(type &tp, json::json &&props = json::json(), std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val = std::nullopt) noexcept;
    void set_value(item &itm, json::json &&val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());
    void delete_item(item &itm) noexcept;

    void create_reactive_rule(std::string_view rule_name, std::string_view rule_content);
    void create_deliberative_rule(std::string_view rule_name, std::string_view rule_content);

    [[nodiscard]] json::json to_json() const noexcept;

  protected:
    void add_property_type(utils::u_ptr<property_type> pt);

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    [[nodiscard]] property_type &get_property_type(std::string_view name) const;

    type &make_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json());

#ifdef BUILD_LISTENERS
  private:
    /**
     * @brief Notifies when the type is created.
     *
     * @param tp The created type.
     */
    void new_type(const type &tp);

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    void new_item(const item &itm);
#endif

  protected:
    coco_db &db;                                                                            // The database..
    json::json schemas;                                                                     // The JSON schemas..
    std::map<std::string, utils::u_ptr<property_type>, std::less<>> property_types;         // The property types..
    std::recursive_mutex mtx;                                                               // The mutex for the core..
    Environment *env;                                                                       // The CLIPS environment..
    std::map<std::string, utils::u_ptr<type>, std::less<>> types;                           // The types managed by CoCo by name.
    std::map<std::string, utils::u_ptr<reactive_rule>, std::less<>> reactive_rules;         // The reactive rules..
    std::map<std::string, utils::u_ptr<deliberative_rule>, std::less<>> deliberative_rules; // The deliberative rules..
#ifdef BUILD_LISTENERS
    std::vector<listener *> listeners; // The CoCo listeners..
#endif
  };

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
    virtual void new_type([[maybe_unused]] const type &tp) {}

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    virtual void new_item([[maybe_unused]] const item &itm) {}

  protected:
    std::recursive_mutex &get_mtx() { return cc.mtx; }

  protected:
    coco &cc;
  };
#endif
} // namespace coco