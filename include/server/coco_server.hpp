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
    std::unique_ptr<network::response> get_type(const network::request &req);
    std::unique_ptr<network::response> create_type(const network::request &req);
    std::unique_ptr<network::response> update_type(const network::request &req);
    std::unique_ptr<network::response> delete_type(const network::request &req);

    std::unique_ptr<network::response> get_items(const network::request &req);
    std::unique_ptr<network::response> get_item(const network::request &req);
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

    virtual void on_ws_open(network::ws_session &ws);
    virtual void on_ws_message(network::ws_session &ws, const std::string &msg);
    virtual void on_ws_close(network::ws_session &ws);
    virtual void on_ws_error(network::ws_session &ws, const std::error_code &);

    virtual void new_type(const type &tp) override;
    virtual void updated_type(const type &tp) override;
    virtual void deleted_type(const std::string &tp_id) override;

    virtual void new_item(const item &itm) override;
    virtual void updated_item(const item &itm) override;
    virtual void new_value(const item &itm) override;
    virtual void deleted_item(const std::string &itm_id) override;

    virtual void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

    virtual void new_solver(const coco_executor &exec) override;
    virtual void deleted_solver(const uintptr_t id) override;

    virtual void new_reactive_rule(const rule &r) override;
    virtual void new_deliberative_rule(const rule &r) override;

    virtual void state_changed(const coco_executor &exec) override;

    virtual void flaw_created(const coco_executor &exec, const ratio::flaw &f) override;
    virtual void flaw_state_changed(const coco_executor &exec, const ratio::flaw &f) override;
    virtual void flaw_cost_changed(const coco_executor &exec, const ratio::flaw &f) override;
    virtual void flaw_position_changed(const coco_executor &exec, const ratio::flaw &f) override;
    virtual void current_flaw(const coco_executor &exec, const ratio::flaw &f) override;

    virtual void resolver_created(const coco_executor &exec, const ratio::resolver &r) override;
    virtual void resolver_state_changed(const coco_executor &exec, const ratio::resolver &r) override;
    virtual void current_resolver(const coco_executor &exec, const ratio::resolver &r) override;

    virtual void causal_link_added(const coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r) override;

    virtual void executor_state_changed(const coco_executor &exec, ratio::executor::executor_state state) override;

    virtual void tick(const coco_executor &exec, const utils::rational &time) override;

  protected:
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
