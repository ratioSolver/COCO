#pragma once

#include "coco.hpp"
#include "server.hpp"

namespace coco
{
  class coco_server : public listener, public network::server
  {
  public:
    coco_server(coco &cc, std::string_view host = SERVER_HOST, unsigned short port = SERVER_PORT);

  protected:
    utils::u_ptr<network::response> index(const network::request &req);
    utils::u_ptr<network::response> assets(const network::request &req);

  private:
    void new_type(const type &tp) override;
    void new_item(const item &itm) override;
  };
} // namespace coco
