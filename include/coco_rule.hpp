#pragma once

#include "json.hpp"

namespace coco
{
  class coco;

  /**
   * @brief Represents a reactive CoCo rule.
   *
   * This class represents a reactive CoCo rule in the form of a name and content.
   */
  class reactive_rule
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
    coco &cc;            // the CoCo core object.
    std::string name;    // the name of the rule.
    std::string content; // the content of the rule.
  };

  /**
   * @brief Represents a deliberative CoCo rule.
   *
   * This class represents a deliberative CoCo rule in the form of a name and content.
   */
  class deliberative_rule
  {
  public:
    /**
     * @brief Constructs a rule object.
     *
     * @param cc The CoCo core object.
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    deliberative_rule(coco &cc, std::string_view name, std::string_view content) noexcept;

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
    coco &cc;            // the CoCo core object.
    std::string name;    // the name of the rule.
    std::string content; // the content of the rule.
  };
} // namespace coco