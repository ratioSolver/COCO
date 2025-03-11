#include "coco_server.hpp"

namespace coco
{
    coco_server::coco_server(coco &cc, std::string_view host, unsigned short port) : listener(cc), server(host, port)
    {
        add_route(network::Get, "^/$", std::bind(&coco_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&coco_server::assets, this, network::placeholders::request));
    }

    utils::u_ptr<network::response> coco_server::index(const network::request &) { return utils::make_u_ptr<network::file_response>(CLIENT_DIR "/dist/index.html"); }
    utils::u_ptr<network::response> coco_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return utils::make_u_ptr<network::file_response>(CLIENT_DIR "/dist" + target);
    }

    void coco_server::new_type(const type &tp)
    {
    }
    void coco_server::new_item(const item &itm)
    {
    }
} // namespace coco
