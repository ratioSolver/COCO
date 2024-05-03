#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <memory>

namespace coco
{
  enum parameter_type
  {
    Integer,
    Real,
    Boolean,
    Symbol,
    String,
    Array
  };

  inline std::string to_string(parameter_type type)
  {
    switch (type)
    {
    case Integer:
      return "Integer";
    case Real:
      return "Real";
    case Boolean:
      return "Boolean";
    case Symbol:
      return "Symbol";
    case String:
      return "String";
    case Array:
      return "Array";
    default:
      return "Unknown";
    }
  }

  class parameter
  {
  public:
    parameter(const std::string &name, parameter_type type) : name(name), type(type) {}
    virtual ~parameter() = default;

    const std::string &get_name() const { return name; }
    parameter_type get_type() const { return type; }

  private:
    std::ostream &operator<<(std::ostream &os) const
    {
      os << "Parameter: " << name << " (" << to_string(type) << ")";
      return os;
    }

  private:
    std::string name;
    parameter_type type;
  };

  class integer_parameter : public parameter
  {
  public:
    integer_parameter(const std::string &name, long min, long max) : parameter(name, parameter_type::Integer), min(min), max(max) {}

    long get_min() const { return min; }
    long get_max() const { return max; }

  private:
    const long min, max;
  };

  class real_parameter : public parameter
  {
  public:
    real_parameter(const std::string &name, double min, double max) : parameter(name, parameter_type::Real), min(min), max(max) {}

    double get_min() const { return min; }
    double get_max() const { return max; }

  private:
    const double min, max;
  };

  class boolean_parameter : public parameter
  {
  public:
    boolean_parameter(const std::string &name) : parameter(name, parameter_type::Boolean) {}
  };

  class symbol_parameter : public parameter
  {
  public:
    symbol_parameter(const std::string &name, const std::vector<std::string> &symbols) : parameter(name, parameter_type::Symbol), symbols(symbols) {}

    const std::vector<std::string> &get_symbols() const { return symbols; }

  private:
    const std::vector<std::string> symbols;
  };

  class string_parameter : public parameter
  {
  public:
    string_parameter(const std::string &name) : parameter(name, parameter_type::String) {}
  };

  class array_parameter : public parameter
  {
  public:
    array_parameter(const std::string &name, std::unique_ptr<parameter> &&type, const std::vector<int> &shape) : parameter(name, parameter_type::Array), type(std::move(type)), shape(shape) {}

    const parameter &get_array_type() const { return *type; }
    const std::vector<int> &get_shape() const { return shape; }

  private:
    const std::unique_ptr<parameter> type;
    const std::vector<int> shape;
  };
} // namespace coco
