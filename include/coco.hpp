#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"

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

  class coco
  {
    friend class type;
    friend class item;
    friend class property;

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
    [[nodiscard]] type &get_type(const std::string &name);

    [[nodiscard]] type &create_type(std::string_view name, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json()) noexcept;

  protected:
    void add_property_type(utils::u_ptr<property_type> pt);

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    [[nodiscard]] property_type &get_property_type(std::string_view name) const;

  private:
    /**
     * @brief Notifies when the type is created.
     *
     * @param tp The created type.
     */
    virtual void new_type(const type &tp);

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    virtual void new_item(const item &itm);

  protected:
    coco_db &db;                                                       // the database..
    json::json schemas;                                                // the JSON schemas..
    std::map<std::string, utils::u_ptr<property_type>> property_types; // the property types..
    Environment *env;                                                  // the CLIPS environment..
    std::map<std::string, utils::u_ptr<type>> types;                   // The types managed by CoCo by name.
  };
} // namespace coco