#pragma once

#include "json.hpp"

namespace coco
{
  class coco_db;
  class coco_core;

  /**
   * @brief Represents a CoCo rule.
   *
   * This class represents a CoCo rule, either reactive or deliberative, in the form of a name and content.
   */
  class rule
  {
    friend class coco_db;

  public:
    /**
     * @brief Constructs a rule object.
     *
     * @param cc The CoCo core object.
     * @param id The ID of the rule.
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    rule(coco_core &cc, const std::string &id, const std::string &name, const std::string &content, bool reactive = true) noexcept;

    /**
     * @brief Gets the ID of the rule.
     *
     * @return The ID of the rule.
     */
    const std::string &get_id() const { return id; }

    /**
     * @brief Gets the name of the rule.
     *
     * @return The name of the rule.
     */
    const std::string &get_name() const { return name; }

    /**
     * @brief Gets the content of the rule.
     *
     * @return The content of the rule.
     */
    const std::string &get_content() const { return content; }

  private:
    coco_core &cc;       // the CoCo core object.
    std::string id;      // the ID of the rule.
    std::string name;    // the name of the rule.
    std::string content; // the content of the rule.
    bool reactive;       // whether the rule is reactive or deliberative.
  };
} // namespace coco
