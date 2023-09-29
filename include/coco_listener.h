#pragma once

#include "coco_core.h"
#include <unordered_set>

namespace coco
{
  class coco_executor;
  class sensor_type;
  class sensor;
  class user;

  class coco_listener
  {
    friend class coco_core;

  public:
    coco_listener(coco_core &cc) : cc(cc) { cc.listeners.push_back(this); }
    coco_listener(const coco_listener &orig) = delete;
    virtual ~coco_listener() { cc.listeners.erase(std::find(cc.listeners.cbegin(), cc.listeners.cend(), this)); }

  private:
    virtual void new_user([[maybe_unused]] const user &u) {}
    virtual void updated_user([[maybe_unused]] const user &u) {}
    virtual void removed_user([[maybe_unused]] const user &u) {}

    virtual void new_sensor_type([[maybe_unused]] const sensor_type &s) {}
    virtual void updated_sensor_type([[maybe_unused]] const sensor_type &s) {}
    virtual void removed_sensor_type([[maybe_unused]] const sensor_type &s) {}

    virtual void new_sensor([[maybe_unused]] const sensor &s) {}
    virtual void updated_sensor([[maybe_unused]] const sensor &s) {}
    virtual void removed_sensor([[maybe_unused]] const sensor &s) {}

    virtual void new_sensor_data([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &time, [[maybe_unused]] const json::json &value) {}
    virtual void new_sensor_state([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &time, [[maybe_unused]] const json::json &state) {}

    virtual void message_arrived([[maybe_unused]] const std::string &topic, [[maybe_unused]] const json::json &msg) {}

    virtual void new_solver([[maybe_unused]] const coco_executor &exec) {}
    virtual void removed_solver([[maybe_unused]] const coco_executor &exec) {}

    virtual void state_changed([[maybe_unused]] const coco_executor &exec) {}

    virtual void started_solving([[maybe_unused]] const coco_executor &exec) {}
    virtual void solution_found([[maybe_unused]] const coco_executor &exec) {}
    virtual void inconsistent_problem([[maybe_unused]] const coco_executor &exec) {}

    virtual void flaw_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void flaw_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void flaw_cost_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void flaw_position_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void current_flaw([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}

    virtual void resolver_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}
    virtual void resolver_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}
    virtual void current_resolver([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}

    virtual void causal_link_added([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r) {}

    virtual void executor_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] ratio::executor::executor_state state) {}

    virtual void tick([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const utils::rational &time) {}

    virtual void start([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::unordered_set<ratio::atom *> &atoms) {}
    virtual void end([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::unordered_set<ratio::atom *> &atoms) {}

  protected:
    coco_core &cc;
  };
} // namespace coco
