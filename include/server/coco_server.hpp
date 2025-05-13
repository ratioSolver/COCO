#pragma once

#include "coco_module.hpp"
#include "server.hpp"
#include <unordered_set>
#include <typeindex>

namespace coco
{
  class coco_server;

  class server_module
  {
  public:
    server_module(coco_server &srv) noexcept;
    virtual ~server_module() = default;

  protected:
    coco &get_coco() noexcept;

  protected:
    coco_server &srv;
  };

  class coco_server : public coco_module, public network::server
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
        auto mod = utils::make_u_ptr<Tp>(std::forward<Args>(args)...);
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

    utils::u_ptr<network::response> get_data(const network::request &req);
    utils::u_ptr<network::response> set_datum(const network::request &req);

    utils::u_ptr<network::response> fake(const network::request &req);

    utils::u_ptr<network::response> get_reactive_rules(const network::request &req);
    utils::u_ptr<network::response> create_reactive_rule(const network::request &req);

    utils::u_ptr<network::response> get_deliberative_rules(const network::request &req);
    utils::u_ptr<network::response> create_deliberative_rule(const network::request &req);

  private:
    std::map<std::type_index, utils::u_ptr<server_module>> modules; // the server modules
  };
} // namespace coco
