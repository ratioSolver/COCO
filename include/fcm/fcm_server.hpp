#pragma once

#include "coco_server.hpp"
#include "coco_fcm.hpp"

namespace coco
{
    class fcm_server : public server_module
    {
    public:
        fcm_server(coco_server &srv, coco_fcm &fcm) noexcept;

    private:
        std::unique_ptr<network::response> new_token(const network::request &req);

    private:
        coco_fcm &fcm; // The fcm module
    };
} // namespace coco
