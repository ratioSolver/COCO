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
  class property;
  class item;

  class property_type
  {
    friend class type;
    friend class item;

  public:
    property_type(coco &cc, std::string_view name) noexcept;
    virtual ~property_type() = default;

    /**
     * @brief Gets the CoCo environment of the property type.
     *
     * @return The CoCo environment of the property type.
     */
    [[nodiscard]] coco &get_coco() const noexcept { return cc; }

    /**
     * @brief Gets the name of the property type.
     *
     * @return The name of the property type.
     */
    [[nodiscard]] const std::string &get_name() const noexcept { return name; }

  private:
    [[nodiscard]] virtual utils::u_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept = 0;

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

  private:
    [[nodiscard]] utils::u_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class int_property_type final : public property_type
  {
  public:
    int_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] utils::u_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class property
  {
    friend class item;

  public:
    property(const property_type &pt, const type &tp, bool dynamic, std::string_view name) noexcept;
    virtual ~property();

    /**
     * @brief Gets the type of the property.
     *
     * @return The type of the property.
     */
    [[nodiscard]] const property_type &get_property_type() const noexcept { return pt; }

    /**
     * @brief Gets the type this property belongs to.
     *
     * @return The type this property belongs to.
     */
    [[nodiscard]] const type &get_type() const noexcept { return tp; }

    /**
     * @brief Gets the dynamicity of the property.
     *
     * @return The dynamicity of the property.
     */
    [[nodiscard]] bool is_dynamic() const noexcept { return dynamic; }

    /**
     * @brief Gets the name of the property.
     *
     * @return The name of the property.
     */
    [[nodiscard]] const std::string &get_name() const noexcept { return name; }

    /**
     * @brief Validates the property against a JSON object and schema references.
     * @param j The JSON object to validate.
     * @return True if the property is valid, false otherwise.
     */
    [[nodiscard]] virtual bool validate(const json::json &j) const noexcept = 0;

  protected:
    [[nodiscard]] std::string get_deftemplate_name() const noexcept;

    Environment *get_env() const noexcept;
    const json::json &get_schemas() const noexcept;

  protected:
    const property_type &pt;
    const type &tp;
    const bool dynamic;
    const std::string name;
  };

  class bool_property final : public property
  {
  public:
    bool_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<bool> default_value = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

  private:
    std::optional<bool> default_value;
  };

  class int_property final : public property
  {
  public:
    int_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<INT_TYPE> default_value = std::nullopt, std::optional<INT_TYPE> min = std::nullopt, std::optional<INT_TYPE> max = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

  private:
    std::optional<INT_TYPE> default_value, min, max;
  };
} // namespace coco
