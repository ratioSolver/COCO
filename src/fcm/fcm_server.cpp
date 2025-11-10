#include "fcm_server.hpp"
#ifdef BUILD_AUTH
#include "coco_auth.hpp"
#endif
#include "logging.hpp"

namespace coco
{
    fcm_server::fcm_server(coco_server &srv, coco_fcm &fcm) noexcept : server_module(srv), fcm(fcm)
    {
        srv.add_route(network::Post, "^/fcm_tokens$", std::bind(&fcm_server::new_token, this, network::placeholders::request));

        // Define OpenAPI paths for FCM endpoints
        add_path("/fcm_tokens", {"post",
                                 {{"summary", "Add a new FCM token."},
                                  {"description", "Endpoint to add a new FCM token for a user."},
                                  {"requestBody",
                                   {{"required", true},
                                    {"content", {{"application/json", {{"schema", {{"type", "object"}, {"properties", {{"id", {{"type", "string"}, {"description", "User ID"}}}, {"token", {{"type", "string"}, {"description", "FCM token"}}}}}}}}}}}}},
                                  {"responses",
                                   {{"204",
                                     {{"description", "Token added successfully."}}},
                                    {"400",
                                     {{"description", "Invalid request."}}}}}}});
#ifdef BUILD_AUTH
        get_path("/fcm_tokens")["post"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        auto &auth = static_cast<auth_middleware &>(srv.get_middleware<auth_middleware>());
        auth.add_authorized_path(network::verb::Post, "^/fcm_tokens$", {0});
#endif
    }

    std::unique_ptr<network::response> fcm_server::new_token(const network::request &req)
    {
        LOG_TRACE(req);
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("id") || !body["id"].is_string() || !body.contains("token") || !body["token"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        std::string token = body["token"];
        fcm.add_token(id, token);
        return std::make_unique<network::response>(network::status_code::no_content);
    }
} // namespace coco