#pragma once

#include "memory.hpp"
#include <string>
#include <vector>

namespace coco
{
  class node
  {
  public:
    explicit node(const std::string &name) : name(name) {}
    virtual ~node() = default;

    const std::string &get_name() const { return name; }

  private:
    const std::string name;
  };

  class utterance_node : public node
  {
  public:
    utterance_node(const std::string &name, const std::vector<std::string> &utterances) : node(name), utterances(utterances) {}

    const std::vector<std::string> &get_utterances() const { return utterances; }

  private:
    const std::vector<std::string> utterances;
  };

  class collect_node : public node
  {
  public:
    collect_node(const std::string &name, const std::string &property) : node(name), property(property) {}

    const std::string &get_property() const { return property; }

  private:
    const std::string property;
  };
} // namespace coco
