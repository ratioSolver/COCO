#pragma once

#include "coco_export.h"
#include "core_listener.h"
#include "solver_listener.h"
#include "executor_listener.h"

namespace coco
{
  class coco_core;

  class coco_executor : public ratio::core::core_listener, public ratio::solver::solver_listener, public ratio::executor::executor_listener
  {
    friend class coco_core;

  public:
    COCO_EXPORT coco_executor(coco_core &cc, ratio::executor::executor &exec, const std::string &type);

    /**
     * @brief Get the executor object associated to this executor.
     *
     * @return ratio::executor::executor&
     */
    ratio::executor::executor &get_executor() { return exec; }

    /**
     * @brief Get the type of the executor.
     *
     * @return const std::string&
     */
    const std::string &get_type() const { return type; }

    /**
     * @brief Start the execution of the current plan.
     *
     */
    void start_execution() { executing = true; }
    /**
     * @brief Pause the execution of the current plan.
     *
     */
    void pause_execution() { executing = false; }

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

    friend COCO_EXPORT json::json to_state(const coco_executor &rhs) noexcept;
    friend COCO_EXPORT json::json to_graph(const coco_executor &rhs) noexcept;

  private:
    coco_core &cc;
    ratio::executor::executor &exec;
    const std::string type;
    bool executing = false;
    std::unordered_set<const ratio::solver::flaw *> flaws;
    const ratio::solver::flaw *c_flaw = nullptr;
    std::unordered_set<const ratio::solver::resolver *> resolvers;
    const ratio::solver::resolver *c_resolver = nullptr;
    semitone::rational current_time;
    std::unordered_set<ratio::core::atom *> executing_atoms;
  };

  inline uintptr_t get_id(const coco_executor &exec) noexcept { return reinterpret_cast<uintptr_t>(&exec); }
} // namespace coco
