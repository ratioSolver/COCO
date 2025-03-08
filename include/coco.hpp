#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"

namespace coco
{
  class coco_db;
  class type;
  class property;

  class coco
  {
    friend class type;

  public:
    coco(coco_db &db) noexcept;

  private:
    /**
     * @brief Creates a property object for the given type from a JSON object.
     *
     * This function takes a JSON object as input and creates a property object for the given type based on the JSON's contents.
     *
     * @param tp The type the property belongs to.
     * @param dynamic The dynamicity of the property.
     * @param name The name of the property.
     * @param j The JSON object containing the property data.
     * @return A unique pointer to the created property object.
     */
    utils::u_ptr<property> make_property(const type &tp, bool dynamic, const std::string &name, const json::json &j);

  protected:
    coco_db &db;      // the database..
    Environment *env; // the CLIPS environment..
  };
} // namespace coco