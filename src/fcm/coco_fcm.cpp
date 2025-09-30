#include "coco_fcm.hpp"
#include "coco.hpp"
#include "fcm_db.hpp"
#include "base64.hpp"
#include "crypto.hpp"
#include "logging.hpp"

namespace coco
{
    coco_fcm::coco_fcm(coco &cc, std::string_view fcm_project_id, std::string_view client_email, std::string_view private_key) noexcept : coco_module(cc), fcm_project_id(fcm_project_id), client_email(client_email), private_key(private_key), access_token_client("oauth2.googleapis.com", 443), client("fcm.googleapis.com", 443)
    {
        [[maybe_unused]] auto send_notification_err = AddUDF(get_env(), "send_notification", "v", 3, 3, "yss", send_notification_udf, "send_notification_udf", this);
        assert(send_notification_err == AUE_NO_ERROR);

        get_coco().get_db().add_module<fcm_db>(static_cast<mongo_db &>(get_coco().get_db()));
    }

    void coco_fcm::add_token(std::string_view id, std::string_view token) { get_coco().get_db().get_module<fcm_db>().add_token(id, token); }

    void coco_fcm::send_notification(std::string_view id, std::string_view title, std::string_view body)
    {
        if (access_token.empty() || std::chrono::system_clock::now() >= access_token_expiration)
            refresh_access_token();
        auto &db = get_coco().get_db().get_module<fcm_db>();
        for (const auto &token : db.get_tokens(id))
        {
            json::json j_message{{"message", {{"token", token}, {"notification", {{"title", title.data()}, {"body", body.data()}}}}}};
            auto res = client.post("/v1/projects/" + fcm_project_id + "/messages:send", std::move(j_message), {{"Content-Type", "application/json"}, {"Authorization", "Bearer " + access_token}});
            if (res)
            {
                if (res->get_status_code() == network::status_code::ok)
                    LOG_DEBUG("Sent FCM notification to token " + token + " for item " + std::string(id));
                else
                {
                    LOG_DEBUG("Failed to send FCM notification to token " + token + " for item " + std::string(id) + ": " + static_cast<network::json_response &>(*res).get_body().dump());
                    get_coco().get_db().get_module<fcm_db>().remove_token(id, token);
                }
            }
            else
                LOG_ERR("Failed to send FCM notification to token " + token + " for item " + std::string(id));
        }
    }

    void coco_fcm::refresh_access_token()
    {
        LOG_DEBUG("Refreshing FCM access token");

        json::json j_header{{"alg", "RS256"}, {"typ", "JWT"}};
        long iat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        long exp = iat + 3600; // 1 hour expiration
        json::json j_claims{{"iss", client_email}, {"scope", "https://www.googleapis.com/auth/firebase.messaging"}, {"aud", "https://oauth2.googleapis.com/token"}, {"iat", iat}, {"exp", exp}};
        auto header = utils::base64url_encode(j_header.dump());
        auto claims = utils::base64url_encode(j_claims.dump());
        auto jwt = header + "." + claims + "." + utils::base64url_encode(utils::sign_rs256(header + "." + claims, private_key));
        std::string body = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt;
        auto res = access_token_client.post("/token", std::move(body), {{"Content-Type", "application/x-www-form-urlencoded"}});
        if (res)
        {
            if (res->get_status_code() == network::status_code::ok)
            {
                auto j_res = static_cast<network::json_response &>(*res).get_body();
                access_token = j_res["access_token"].get<std::string>();
                access_token_expiration = std::chrono::system_clock::now() + std::chrono::seconds(j_res["expires_in"].get<int>());
                LOG_DEBUG("Refreshed FCM access token, expires at " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(access_token_expiration.time_since_epoch()).count()));
            }
            else
                LOG_ERR("Failed to refresh FCM access token: " + static_cast<network::json_response &>(*res).get_body().dump());
        }
        else
            LOG_ERR("Failed to refresh FCM access token");
    }

    void send_notification_udf(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Sending notification via FCM");

        auto &fcm = *reinterpret_cast<coco_fcm *>(udfc->context);

        UDFValue user_id; // we get the user ID from the first argument
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &user_id))
            return;

        UDFValue title; // we get the title from the second argument
        if (!UDFNextArgument(udfc, STRING_BIT, &title))
            return;

        UDFValue body; // we get the body from the third argument
        if (!UDFNextArgument(udfc, STRING_BIT, &body))
            return;

        fcm.send_notification(user_id.lexemeValue->contents, title.lexemeValue->contents, body.lexemeValue->contents);
    }
} // namespace coco