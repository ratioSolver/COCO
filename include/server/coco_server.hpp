#pragma once

#include "coco.hpp"
#include "server.hpp"
#include <unordered_set>

namespace coco
{
  class coco_server : public listener, public network::server
  {
  public:
    coco_server(coco &cc, std::string_view host = SERVER_HOST, unsigned short port = SERVER_PORT);

  protected:
    utils::u_ptr<network::response> index(const network::request &req);
    utils::u_ptr<network::response> assets(const network::request &req);

    utils::u_ptr<network::response> get_types(const network::request &req);
    utils::u_ptr<network::response> get_type(const network::request &req);
    utils::u_ptr<network::response> create_type(const network::request &req);
    utils::u_ptr<network::response> delete_type(const network::request &req);

    utils::u_ptr<network::response> get_items(const network::request &req);
    utils::u_ptr<network::response> get_item(const network::request &req);
    utils::u_ptr<network::response> create_item(const network::request &req);
    utils::u_ptr<network::response> delete_item(const network::request &req);

    utils::u_ptr<network::response> get_values(const network::request &req);
    utils::u_ptr<network::response> set_value(const network::request &req);

    utils::u_ptr<network::response> fake(const network::request &req);

    utils::u_ptr<network::response> get_reactive_rules(const network::request &req);
    utils::u_ptr<network::response> create_reactive_rule(const network::request &req);
    utils::u_ptr<network::response> delete_reactive_rule(const network::request &req);

    utils::u_ptr<network::response> get_deliberative_rules(const network::request &req);
    utils::u_ptr<network::response> create_deliberative_rule(const network::request &req);
    utils::u_ptr<network::response> delete_deliberative_rule(const network::request &req);

  private:
    virtual void on_ws_open(network::ws_session &ws);
    virtual void on_ws_message(network::ws_session &ws, std::string_view msg);
    virtual void on_ws_close(network::ws_session &ws);
    virtual void on_ws_error(network::ws_session &ws, const std::error_code &);

    void new_type(const type &tp) override;
    void new_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

    void state_changed(coco_executor &exec) override;

    void flaw_created(coco_executor &exec, const ratio::flaw &f) override;
    void flaw_state_changed(coco_executor &exec, const ratio::flaw &f) override;
    void flaw_cost_changed(coco_executor &exec, const ratio::flaw &f) override;
    void flaw_position_changed(coco_executor &exec, const ratio::flaw &f) override;
    void current_flaw(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::flaw>> f) override;
    void resolver_created(coco_executor &exec, const ratio::resolver &r) override;
    void resolver_state_changed(coco_executor &exec, const ratio::resolver &r) override;
    void current_resolver(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::resolver>> r) override;
    void causal_link_added(coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r) override;

    void executor_state_changed(coco_executor &exec, ratio::executor::executor_state state) override;
    void tick(coco_executor &exec, const utils::rational &time) override;
    void starting(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) override;
    void start(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) override;
    void ending(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) override;
    void end(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) override;

  protected:
    void broadcast(json::json &&msg);

  private:
    std::unordered_set<network::ws_session *> clients;
  };
} // namespace coco
