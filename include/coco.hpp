#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"

namespace coco
{
  constexpr const char *type_deftemplate = "(deftemplate type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)))";
  constexpr const char *is_a_deftemplate = "(deftemplate is_a (slot type_id (type SYMBOL)) (slot parent_id (type SYMBOL)))";
  constexpr const char *item_deftemplate = "(deftemplate item (slot id (type SYMBOL)))";
  constexpr const char *instance_of_deftemplate = "(deftemplate instance_of (slot item_id (type SYMBOL)) (slot type_id (type SYMBOL)))";
  constexpr const char *inheritance_rule = "(defrule inheritance (is_a (type_id ?t) (parent_id ?p)) (instance_of (item_id ?i) (type_id ?t)) => (assert (instance_of (item_id ?i) (type_id ?p))))";
  constexpr const char *all_instances_of_function = "(deffunction all-instances-of (?type_id) (bind ?instances (create$)) (do-for-all-facts ((?instance_of instance_of)) (eq ?instance_of:type_id ?type_id) (bind ?instances (create$ ?instances ?instance_of:item_id))) (return ?instances))";

  class coco_db;
  class type;
  class property_type;

  class coco
  {
    friend class type;
    friend class property_type;

  public:
    coco(coco_db &db) noexcept;
    ~coco();

  protected:
    void add_property_type(utils::u_ptr<property_type> pt);

    std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    property_type &get_property_type(std::string_view name) const;

  protected:
    coco_db &db;                                                       // the database..
    std::map<std::string, utils::u_ptr<property_type>> property_types; // the property types..
    Environment *env;                                                  // the CLIPS environment..
  };
} // namespace coco