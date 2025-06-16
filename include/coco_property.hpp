#pragma once

#include "json.hpp"
#include "clips.h"
#include <memory>
#include <random>
#include <optional>

namespace coco
{
  constexpr const char *bool_kw = "bool";
  constexpr const char *int_kw = "int";
  constexpr const char *float_kw = "float";
  constexpr const char *string_kw = "string";
  constexpr const char *symbol_kw = "symbol";
  constexpr const char *item_kw = "item";
  constexpr const char *json_kw = "json";

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
    [[nodiscard]] virtual std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept = 0;

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
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class int_property_type final : public property_type
  {
  public:
    int_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class float_property_type final : public property_type
  {
  public:
    float_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class string_property_type final : public property_type
  {
  public:
    string_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class symbol_property_type final : public property_type
  {
  public:
    symbol_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class item_property_type final : public property_type
  {
  public:
    item_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

    void set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept override;
  };

  class json_property_type final : public property_type
  {
  public:
    json_property_type(coco &cc) noexcept;

  private:
    [[nodiscard]] std::unique_ptr<property> new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept override;

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

    [[nodiscard]] virtual bool is_complex() const noexcept = 0;

    [[nodiscard]] virtual json::json to_json() const noexcept = 0;

    [[nodiscard]] virtual json::json fake() const noexcept = 0;

  protected:
    [[nodiscard]] std::string get_deftemplate_name() const noexcept;

    Environment *get_env() const noexcept;
    const json::json &get_schemas() const noexcept;
    std::mt19937 &get_gen() const noexcept;

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

    [[nodiscard]] bool is_complex() const noexcept override { return false; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    std::optional<bool> default_value; // The default value for the property.
  };

  class int_property final : public property
  {
  public:
    int_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<long> default_value = std::nullopt, std::optional<long> min = std::nullopt, std::optional<long> max = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

    [[nodiscard]] bool is_complex() const noexcept override { return false; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    std::optional<long> default_value; // The default value for the property.
    std::optional<long> min;           // The minimum value allowed for the property.
    std::optional<long> max;           // The maximum value allowed for the property.
  };

  class float_property final : public property
  {
  public:
    float_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<double> default_value = std::nullopt, std::optional<double> min = std::nullopt, std::optional<double> max = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

    [[nodiscard]] bool is_complex() const noexcept override { return false; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    std::optional<double> default_value; // The default value for the property.
    std::optional<double> min;           // The minimum value allowed for the property.
    std::optional<double> max;           // The maximum value allowed for the property.
  };

  class string_property final : public property
  {
  public:
    string_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<std::string> default_value = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

    [[nodiscard]] bool is_complex() const noexcept override { return false; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    std::optional<std::string> default_value; // The default value for the property.
  };

  class symbol_property final : public property
  {
  public:
    symbol_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple = false, std::vector<std::string> &&values = {}, std::optional<std::vector<std::string>> default_value = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

    [[nodiscard]] bool is_complex() const noexcept override { return multiple; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    bool multiple;                                         // Indicates whether the property can have multiple values.
    std::vector<std::string> values;                       // The possible values for the property.
    std::optional<std::vector<std::string>> default_value; // The default value for the property.
  };

  class item_property final : public property
  {
  public:
    item_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, const type &domain, bool multiple = false, std::optional<std::vector<std::reference_wrapper<item>>> default_value = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

    [[nodiscard]] bool is_complex() const noexcept override { return multiple; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    const type &domain;                                                     // The domain of the property.
    bool multiple;                                                          // Indicates whether the property can have multiple values.
    std::optional<std::vector<std::reference_wrapper<item>>> default_value; // The default value for the property.
  };

  class json_property final : public property
  {
  public:
    json_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<json::json> schema = std::nullopt, std::optional<json::json> default_value = std::nullopt) noexcept;

    [[nodiscard]] bool validate(const json::json &j) const noexcept override;

    [[nodiscard]] bool is_complex() const noexcept override { return true; }

    [[nodiscard]] json::json to_json() const noexcept override;

    [[nodiscard]] json::json fake() const noexcept override;

  private:
    std::optional<json::json> schema;        // The validation schema..
    std::optional<json::json> default_value; // The default value for the property.
  };
} // namespace coco
