#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#include <optional>

namespace coco
{
  constexpr const char *bool_kw = "bool";
  constexpr const char *int_kw = "int";
  constexpr const char *real_kw = "real";

  class coco;
  class type;

  class property_type
  {
    friend class type;

  public:
    property_type(coco &cc, std::string_view name) noexcept;
    virtual ~property_type() = default;

    /**
     * @brief Gets the name of the property type.
     *
     * @return The name of the property type.
     */
    [[nodiscard]] const std::string &get_name() const noexcept { return name; }

    /**
     * @brief Validates the property against a JSON object and schema references.
     * @param j The JSON object to validate.
     * @param schema_refs The schema references to use for validation.
     * @return True if the property is valid, false otherwise.
     */
    virtual bool validate(const json::json &j, const json::json &schema_refs) const noexcept = 0;

  protected:
    Environment *get_env();

  private:
    virtual void make_static_property(type &tp, std::string_view name, const json::json &j) noexcept = 0;
    virtual void make_dynamic_property(type &tp, std::string_view name, const json::json &j) noexcept = 0;

    /**
     * Sets the value of the property.
     *
     * This function is responsible for setting the value of the property based on the provided `value`.
     *
     * @param property_fact_builder A pointer to the FactBuilder object.
     * @param name The name of the property to be set.
     * @param value The value to be set for the property.
     */
    virtual void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept = 0;

  protected:
    coco &cc;
    const std::string name;
  };

  class bool_property_type final : public property_type
  {
  public:
    bool_property_type(coco &cc) noexcept;

    bool validate(const json::json &j, const json::json &schema_refs) const noexcept;

  private:
    void make_static_property(type &tp, std::string_view name, const json::json &j) noexcept override;
    void make_dynamic_property(type &tp, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };
} // namespace coco
