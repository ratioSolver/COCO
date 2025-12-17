#pragma once

#include "json.hpp"

namespace coco
{
  class coco;

  /**
   * @brief Represents a CoCo rule.
   *
   * This class represents a CoCo rule in the form of a name and content.
   */
  class rule final
  {
  public:
    /**
     * @brief Constructs a rule object.
     *
     * @param cc The CoCo core object.
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    rule(coco &cc, std::string_view name, std::string_view content) noexcept;
    ~rule();

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
} // namespace coco
