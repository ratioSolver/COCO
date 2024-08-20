#pragma once

#include "coco_db.hpp"
#include "coco_core.hpp"
#include "server.hpp"

namespace coco
{
  class coco_server : public coco::coco_core, private network::server
  {
  public:
    coco_server(std::unique_ptr<coco::coco_db> db = nullptr);

    void start() { network::server::start(); }

  private:
    std::unique_ptr<network::response> index(const network::request &req);
    std::unique_ptr<network::response> assets(const network::request &req);
    std::unique_ptr<network::response> open_api(const network::request &req);
    std::unique_ptr<network::response> async_api(const network::request &req);

    std::unique_ptr<network::response> get_types(const network::request &req);
    std::unique_ptr<network::response> create_type(const network::request &req);
    std::unique_ptr<network::response> update_type(const network::request &req);
    std::unique_ptr<network::response> delete_type(const network::request &req);

    std::unique_ptr<network::response> get_items(const network::request &req);
    std::unique_ptr<network::response> create_item(const network::request &req);
    std::unique_ptr<network::response> update_item(const network::request &req);
    std::unique_ptr<network::response> delete_item(const network::request &req);

    std::unique_ptr<network::response> get_data(const network::request &req);
    std::unique_ptr<network::response> add_data(const network::request &req);

    std::unique_ptr<network::response> get_reactive_rules(const network::request &req);
    std::unique_ptr<network::response> create_reactive_rule(const network::request &req);
    std::unique_ptr<network::response> update_reactive_rule(const network::request &req);
    std::unique_ptr<network::response> delete_reactive_rule(const network::request &req);

    std::unique_ptr<network::response> get_deliberative_rules(const network::request &req);
    std::unique_ptr<network::response> create_deliberative_rule(const network::request &req);
    std::unique_ptr<network::response> update_deliberative_rule(const network::request &req);
    std::unique_ptr<network::response> delete_deliberative_rule(const network::request &req);

    void on_ws_open(network::ws_session &ws);
    void on_ws_message(network::ws_session &ws, const std::string &msg);
    void on_ws_close(network::ws_session &ws);
    void on_ws_error(network::ws_session &ws, const boost::system::error_code &);

    void new_type(const type &tp) override;
    void updated_type(const type &tp) override;
    void deleted_type(const std::string &tp_id) override;

    void new_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void deleted_item(const std::string &itm_id) override;

    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

    void new_solver(const coco_executor &exec) override;
    void deleted_solver(const uintptr_t id) override;

    void new_reactive_rule(const rule &r) override;
    void new_deliberative_rule(const rule &r) override;

    void state_changed(const coco_executor &exec) override;

    void flaw_created(const coco_executor &exec, const ratio::flaw &f) override;
    void flaw_state_changed(const coco_executor &exec, const ratio::flaw &f) override;
    void flaw_cost_changed(const coco_executor &exec, const ratio::flaw &f) override;
    void flaw_position_changed(const coco_executor &exec, const ratio::flaw &f) override;
    void current_flaw(const coco_executor &exec, const ratio::flaw &f) override;

    void resolver_created(const coco_executor &exec, const ratio::resolver &r) override;
    void resolver_state_changed(const coco_executor &exec, const ratio::resolver &r) override;
    void current_resolver(const coco_executor &exec, const ratio::resolver &r) override;

    void causal_link_added(const coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r) override;

    void executor_state_changed(const coco_executor &exec, ratio::executor::executor_state state) override;

    void tick(const coco_executor &exec, const utils::rational &time) override;

  private:
    void broadcast(const json::json &msg)
    {
      auto msg_str = msg.dump();
      for (auto client : clients)
        client->send(msg_str);
    }

  private:
    std::unordered_set<network::ws_session *> clients;
  };

  [[nodiscard]] json::json build_open_api() noexcept;
  [[nodiscard]] json::json build_async_api() noexcept;
} // namespace coco
