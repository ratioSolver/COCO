#pragma once

#include "core_listener.h"
#include "solver_listener.h"
#include "executor_listener.h"

namespace coco
{
  class sensor_network;

  class coco_executor : public ratio::core::core_listener, public ratio::solver::solver_listener, public ratio::executor::executor_listener
  {
    friend class sensor_network;

  public:
    coco_executor(sensor_network &sn, ratio::executor::executor &exec);

    ratio::executor::executor &get_executor() { return exec; }

  private:
    void log(const std::string &msg) override;
    void read(const std::string &script) override;
    void read(const std::vector<std::string> &files) override;

    void state_changed() override;

    void started_solving() override;
    void solution_found() override;
    void inconsistent_problem() override;

  private:
    void flaw_created(const ratio::solver::flaw &f) override;
    void flaw_state_changed(const ratio::solver::flaw &f) override;
    void flaw_cost_changed(const ratio::solver::flaw &f) override;
    void flaw_position_changed(const ratio::solver::flaw &f) override;
    void current_flaw(const ratio::solver::flaw &f) override;

    void resolver_created(const ratio::solver::resolver &r) override;
    void resolver_state_changed(const ratio::solver::resolver &r) override;
    void current_resolver(const ratio::solver::resolver &r) override;

    void causal_link_added(const ratio::solver::flaw &f, const ratio::solver::resolver &r) override;

  private:
    void tick();
    void tick(const semitone::rational &time) override;
    void starting(const std::unordered_set<ratio::core::atom *> &atoms) override;
    void start(const std::unordered_set<ratio::core::atom *> &atoms) override;
    void ending(const std::unordered_set<ratio::core::atom *> &atoms) override;
    void end(const std::unordered_set<ratio::core::atom *> &atoms) override;

    std::string to_task(const ratio::core::atom &atm, const std::string &command);

  private:
    sensor_network &sn;
    ratio::executor::executor &exec;
    bool solved = false;
    std::unordered_set<const ratio::solver::flaw *> flaws;
    const ratio::solver::flaw *c_flaw = nullptr;
    std::unordered_set<const ratio::solver::resolver *> resolvers;
    const ratio::solver::resolver *c_resolver = nullptr;
    semitone::rational current_time;
    std::unordered_set<ratio::core::atom *> executing;
  };
} // namespace coco
