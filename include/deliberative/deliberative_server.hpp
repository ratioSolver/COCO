#pragma once

#include "coco_server.hpp"
#include "coco_deliberative.hpp"

namespace coco
{
  class deliberative_server : public server_module, public deliberative_listener
  {
  public:
    deliberative_server(coco_server &srv, coco_deliberative &cd) noexcept;

  private:
    std::unique_ptr<network::response> get_deliberative_rules(const network::request &);
    std::unique_ptr<network::response> create_deliberative_rule(const network::request &);

  private:
    void executor_created(coco_executor &exec) override;
    void executor_deleted(coco_executor &exec) override;

    void state_changed(coco_executor &exec) override;

    void flaw_created(coco_executor &exec, const ratio::flaw &f) override;
    void flaw_state_changed(coco_executor &exec, const ratio::flaw &f) override;
    void flaw_cost_changed(coco_executor &exec, const ratio::flaw &f) override;
    void flaw_position_changed(coco_executor &exec, const ratio::flaw &f) override;
    void current_flaw(coco_executor &exec, std::optional<std::reference_wrapper<ratio::flaw>> f) override;
    void resolver_created(coco_executor &exec, const ratio::resolver &r) override;
    void resolver_state_changed(coco_executor &exec, const ratio::resolver &r) override;
    void current_resolver(coco_executor &exec, std::optional<std::reference_wrapper<ratio::resolver>> r) override;
    void causal_link_added(coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r) override;

    void executor_state_changed(coco_executor &exec, ratio::executor::executor_state state) override;
    void tick(coco_executor &exec, const utils::rational &time) override;
    void starting(coco_executor &exec, const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) override;
    void start(coco_executor &exec, const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) override;
    void ending(coco_executor &exec, const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) override;
    void end(coco_executor &exec, const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) override;
  };
} // namespace coco
