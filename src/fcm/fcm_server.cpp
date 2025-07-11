#include "fcm_server.hpp"

namespace coco
{
    fcm_server::fcm_server(coco_server &srv, coco_fcm &fcm) noexcept : server_module(srv), fcm(fcm)
    {
        srv.add_route(network::Post, "^/fcm_tokens$", std::bind(&fcm_server::new_token, this, network::placeholders::request));
    }

    std::unique_ptr<network::response> fcm_server::new_token(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("id") || !body["id"].is_string() || !body.contains("token") || !body["token"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        std::string token = body["token"];
        try
        {
            fcm.add_token(id, token);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
} // namespace coco