#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"

namespace coco
{
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