#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"

namespace coco
{
  constexpr const char *type_deftemplate = "(deftemplate type (slot name (type STRING)))";
  constexpr const char *is_a_deftemplate = "(deftemplate is_a (slot type (type STRING)) (slot parent (type STRING)))";
  constexpr const char *item_deftemplate = "(deftemplate item (slot id (type SYMBOL)))";
  constexpr const char *instance_of_deftemplate = "(deftemplate instance_of (slot id (type SYMBOL)) (slot type (type SYMBOL)))";
  constexpr const char *inheritance_rule = "(defrule inheritance (is_a (type ?t) (parent ?p)) (instance_of (id ?i) (type ?t)) => (assert (instance_of (id ?i) (type ?p))))";
  constexpr const char *all_instances_of_function = "(deffunction all-instances-of (?type) (bind ?instances (create$)) (do-for-all-facts ((?instance_of instance_of)) (eq ?instance_of:type ?type) (bind ?instances (create$ ?instances ?instance_of:id))) (return ?instances))";

  class coco_db;
  class type;
  class item;
  class property_type;

  class coco
  {
    friend class type;
    friend class item;
    friend class property_type;

  public:
    coco(coco_db &db) noexcept;
    ~coco();

  protected:
    void add_property_type(utils::u_ptr<property_type> pt);

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    [[nodiscard]] property_type &get_property_type(std::string_view name) const;

  protected:
    coco_db &db;                                                       // the database..
    std::map<std::string, utils::u_ptr<property_type>> property_types; // the property types..
    Environment *env;                                                  // the CLIPS environment..
  };
} // namespace coco