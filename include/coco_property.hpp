#pragma once

#include "json.hpp"
#include <limits>
#include <memory>

namespace coco
{
  class property
  {
  public:
    property(const std::string &name, const std::string &description) noexcept;
    virtual ~property() = default;

    friend json::json to_json(const property &p) noexcept;

  protected:
    virtual json::json to_json() const noexcept;

  private:
    std::string name;
    std::string description;
  };

  class integer_property : public property
  {
  public:
    integer_property(const std::string &name, const std::string &description, long min = std::numeric_limits<long>::min(), long max = std::numeric_limits<long>::max()) noexcept;

  private:
    json::json to_json() const noexcept override;

  private:
    long min;
    long max;
  };

  class float_property : public property
  {
  public:
    float_property(const std::string &name, const std::string &description, double min = -std::numeric_limits<double>::max(), double max = std::numeric_limits<double>::max()) noexcept;

  private:
    json::json to_json() const noexcept override;

  private:
    double min;
    double max;
  };

  class string_property : public property
  {
  public:
    string_property(const std::string &name, const std::string &description) noexcept;

  private:
    json::json to_json() const noexcept override;
  };

  class symbol_property : public property
  {
  public:
    symbol_property(const std::string &name, const std::string &description) noexcept;

  private:
    json::json to_json() const noexcept override;
  };

  class json_property : public property
  {
  public:
    json_property(const std::string &name, const std::string &description, json::json &&schema) noexcept;

  private:
    json::json to_json() const noexcept override;

  private:
    json::json schema;
  };

  inline json::json to_json(const property &p) noexcept { return p.to_json(); }

  std::unique_ptr<property> make_property(const json::json &j);
} // namespace coco
