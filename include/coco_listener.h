#pragma once

#include "coco.h"
#include "rational.h"
#include "item.h"
#include <unordered_set>

namespace coco
{
  class coco_listener
  {
    friend class coco;

  public:
    coco_listener(coco &cc) : cc(cc) { cc.listeners.push_back(this); }
    coco_listener(const coco_listener &orig) = delete;
    virtual ~coco_listener() { cc.listeners.erase(std::find(cc.listeners.cbegin(), cc.listeners.cend(), this)); }

    coco &get_coco() { return cc; }

  private:
    virtual void message_arrived([[maybe_unused]] const std::string &topic, [[maybe_unused]] json::json &msg) {}

    virtual void tick([[maybe_unused]] const semitone::rational &time) {}

    virtual void start([[maybe_unused]] const std::unordered_set<ratio::core::atom *> &atoms) {}
    virtual void end([[maybe_unused]] const std::unordered_set<ratio::core::atom *> &atoms) {}

  protected:
    coco &cc;
  };
} // namespace coco
