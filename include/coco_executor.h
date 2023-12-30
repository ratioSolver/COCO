#pragma once

#include "coco_export.h"
#include "core_listener.h"
#include "solver_listener.h"
#include "executor_listener.h"
#include <memory>

namespace coco
{
  class coco_core;

  class coco_executor : public riddle::core_listener, public ratio::solver_listener, public ratio::executor::executor_listener
  {
    friend class coco_core;

  public:
    COCO_EXPORT coco_executor(coco_core &cc, ratio::executor::executor &exec);

    /**
     * @brief Get the core object associated to this executor.
     *
     * @return coco_core& the core object
     */
    coco_core &get_core() { return cc; }

    /**
     * @brief Get the executor object associated to this executor.
     *
     * @return ratio::executor::executor&
     */
    ratio::executor::executor &get_executor() { return exec; }
    /**
     * @brief Get the executor object associated to this executor.
     *
     * @return const ratio::executor::executor&
     */
    const ratio::executor::executor &get_executor() const { return exec; }

  private:
    void log(const std::string &msg) override;
    void read(const std::string &script) override;
    void read(const std::vector<std::string> &files) override;

    void state_changed() override;

    void started_solving() override;
    void solution_found() override;
    void inconsistent_problem() override;

  private:
    void flaw_created(const ratio::flaw &f) override;
    void flaw_state_changed(const ratio::flaw &f) override;
    void flaw_cost_changed(const ratio::flaw &f) override;
    void flaw_position_changed(const ratio::flaw &f) override;
    void current_flaw(const ratio::flaw &f) override;

    void resolver_created(const ratio::resolver &r) override;
    void resolver_state_changed(const ratio::resolver &r) override;
    void current_resolver(const ratio::resolver &r) override;

    void causal_link_added(const ratio::flaw &f, const ratio::resolver &r) override;

  private:
    void tick();

    void executor_state_changed(ratio::executor::executor_state state) override;
    void tick(const utils::rational &time) override;

    void starting(const std::unordered_set<ratio::atom *> &atoms) override;
    void start(const std::unordered_set<ratio::atom *> &atoms) override;

    void ending(const std::unordered_set<ratio::atom *> &atoms) override;
    void end(const std::unordered_set<ratio::atom *> &atoms) override;

    friend COCO_EXPORT json::json to_state(const coco_executor &rhs) noexcept;
    friend COCO_EXPORT json::json to_graph(const coco_executor &rhs) noexcept;

  private:
    coco_core &cc;
    ratio::executor::executor &exec;
    std::unordered_set<const ratio::flaw *> flaws;
    const ratio::flaw *c_flaw = nullptr;
    std::unordered_set<const ratio::resolver *> resolvers;
    const ratio::resolver *c_resolver = nullptr;
    utils::rational current_time;
    std::unordered_set<ratio::atom *> executing_atoms;
  };

  using coco_executor_ptr = std::unique_ptr<coco_executor>;

  inline uintptr_t get_id(const coco_executor &exec) noexcept { return reinterpret_cast<uintptr_t>(&exec); }

  inline json::json executor_graph_message(const coco_executor &rhs) noexcept
  {
    json::json j_msg = to_graph(rhs);
    j_msg["type"] = "graph";
    j_msg["solver_id"] = get_id(rhs.get_executor().get_solver());
    return j_msg;
  }
} // namespace coco
