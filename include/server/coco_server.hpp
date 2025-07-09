#pragma once

#include "coco_module.hpp"
#include "coco.hpp"
#include "server.hpp"
#include <unordered_set>
#include <typeindex>

namespace coco
{
  class coco_server;

  class server_module
  {
    friend class coco_server;

  public:
    server_module(coco_server &srv) noexcept;
    virtual ~server_module() = default;

  protected:
    [[nodiscard]] coco &get_coco() noexcept;

  private:
    virtual void on_ws_open(network::ws_server_session_base &) {}
    virtual void on_ws_message(network::ws_server_session_base &, const network::message &) {}
    virtual void on_ws_close(network::ws_server_session_base &) {}
    virtual void on_ws_error(network::ws_server_session_base &, const std::error_code &) {}
    virtual void broadcast(json::json &) {}

  protected:
    coco_server &srv;
  };

#ifdef ENABLE_SSL
  class coco_server : public coco_module, public listener, public network::ssl_server
#else
  class coco_server : public coco_module, public listener, public network::server
#endif
  {
    friend class server_module;

  public:
    coco_server(coco &cc, std::string_view host = SERVER_HOST, unsigned short port = SERVER_PORT);

    template <typename Tp, typename... Args>
    Tp &add_module(Args &&...args)
    {
      static_assert(std::is_base_of<server_module, Tp>::value, "Extension must be derived from server_module");
      if (auto it = modules.find(typeid(Tp)); it == modules.end())
      {
        auto mod = std::make_unique<Tp>(std::forward<Args>(args)...);
        auto &ref = *mod;
        modules.emplace(typeid(Tp), std::move(mod));
        return ref;
      }
      else
        throw std::runtime_error("Module already exists");
    }

    template <typename Tp>
    [[nodiscard]] Tp &get_module() const
    {
      static_assert(std::is_base_of<server_module, Tp>::value, "Extension must be derived from server_module");
      if (auto it = modules.find(typeid(Tp)); it != modules.end())
        return *static_cast<Tp *>(it->second.get());
      throw std::runtime_error("Module not found");
    }

    void broadcast(json::json &&msg);

  private:
    void created_type(const type &tp) override;
    void created_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

  private:
#ifdef BUILD_WEB_APP
    std::unique_ptr<network::response> index(const network::request &req);
    std::unique_ptr<network::response> assets(const network::request &req);
#endif

    std::unique_ptr<network::response> get_types(const network::request &req);
    std::unique_ptr<network::response> get_type(const network::request &req);
    std::unique_ptr<network::response> create_type(const network::request &req);
    std::unique_ptr<network::response> delete_type(const network::request &req);

    std::unique_ptr<network::response> get_items(const network::request &req);
    std::unique_ptr<network::response> get_item(const network::request &req);
    std::unique_ptr<network::response> create_item(const network::request &req);
    std::unique_ptr<network::response> delete_item(const network::request &req);

    std::unique_ptr<network::response> get_data(const network::request &req);
    std::unique_ptr<network::response> set_datum(const network::request &req);

    std::unique_ptr<network::response> fake(const network::request &req);

    std::unique_ptr<network::response> get_reactive_rules(const network::request &req);
    std::unique_ptr<network::response> create_reactive_rule(const network::request &req);

    std::unique_ptr<network::response> get_deliberative_rules(const network::request &req);
    std::unique_ptr<network::response> create_deliberative_rule(const network::request &req);

    std::unique_ptr<network::response> get_openapi_spec(const network::request &req);
    std::unique_ptr<network::response> get_asyncapi_spec(const network::request &req);

  private:
    std::map<std::type_index, std::unique_ptr<server_module>> modules; // the server modules

  protected:
    json::json openapi_spec;  // OpenAPI specification for the server
    json::json asyncapi_spec; // AsyncAPI specification for the server
  };
} // namespace coco
