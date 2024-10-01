#pragma once

#include "coco_db.hpp"
#include "coco_core.hpp"
#include "server.hpp"

namespace coco
{
  class coco_server : public coco::coco_core, public network::server
  {
  public:
#ifdef ENABLE_TRANSFORMER
    coco_server(const std::string &host = SERVER_HOST, unsigned short port = SERVER_PORT, std::unique_ptr<coco::coco_db> db = nullptr, const std::string &transformer_host = TRANSFORMER_HOST, unsigned short transformer_port = TRANSFORMER_PORT);
#else
    coco_server(const std::string &host = SERVER_HOST, unsigned short port = SERVER_PORT, std::unique_ptr<coco::coco_db> db = nullptr);
#endif

  private:
    /**
     * @brief Initializes the routes for the server.
     *
     * This function is responsible for setting up the various routes
     * that the server will handle. It should be overridden by derived
     * classes to define specific routes and their corresponding handlers.
     */
    virtual void init_routes();

  protected:
    std::unique_ptr<network::response> index(const network::request &req);
    std::unique_ptr<network::response> assets(const network::request &req);
    std::unique_ptr<network::response> open_api(const network::request &req);
    std::unique_ptr<network::response> async_api(const network::request &req);

#ifdef ENABLE_AUTH
    std::unique_ptr<network::response> login(const network::request &req);
    virtual std::unique_ptr<network::response> create_user(const network::request &req);
    virtual std::unique_ptr<network::response> update_user(const network::request &req);
    std::unique_ptr<network::response> delete_user(const network::request &req);
#endif

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

#ifdef ENABLE_AUTH
    /**
     * @brief Authorizes a network request based on specified roles.
     *
     * @param req The network request to be authorized.
     * @param roles A set of integer roles that are allowed to authorize the request.
     * @param exceptions A set of string exceptions that may bypass the role check (optional).
     * @return std::unique_ptr<network::response> A unique pointer to the network response indicating the result of the authorization.
     */
    std::unique_ptr<network::response> authorize(const network::request &req, const std::set<int> &roles, std::set<std::string> exceptions = {});
#endif

  private:
    virtual void on_ws_open(network::ws_session &ws);
    virtual void on_ws_message(network::ws_session &ws, const std::string &msg);
    virtual void on_ws_close(network::ws_session &ws);
    virtual void on_ws_error(network::ws_session &ws, const std::error_code &);

    virtual void new_type(const type &tp) override;
    virtual void updated_type(const type &tp) override;
    virtual void deleted_type(const std::string &tp_id) override;

    virtual void new_item(const item &itm) override;
    virtual void updated_item(const item &itm) override;
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
#ifdef ENABLE_AUTH
    void broadcast(const json::json &msg, const std::set<int> &roles = {})
    {
      auto msg_str = msg.dump();
      if (roles.empty())
        for (auto client : clients)
          client.first->send(msg_str);
      else
        for (auto client : clients)
          if (users.find(client.second) != users.end())
            for (auto role : roles)
              if (users.at(client.second) == role)
              { // User has the required role
                client.first->send(msg_str);
                break;
              }
    }
#else
    void broadcast(const json::json &msg)
    {
      auto msg_str = msg.dump();
      for (auto client : clients)
        client->send(msg_str);
    }
#endif

  private:
#ifdef ENABLE_AUTH
    std::unordered_map<network::ws_session *, std::string> clients;
    std::unordered_map<std::string, std::set<network::ws_session *>> devices;
    std::unordered_map<std::string, int> users;
#else
    std::unordered_set<network::ws_session *> clients;
#endif
  };

  [[nodiscard]] json::json build_open_api() noexcept;
  [[nodiscard]] json::json build_async_api() noexcept;
} // namespace coco
