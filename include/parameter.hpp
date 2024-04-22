#pragma once

#include <string>
#include <iostream>

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
} // namespace coco
