#include "coco_fcm.hpp"
#include "base64.hpp"
#include "crypto.hpp"
#include "logging.hpp"

namespace coco
{
    coco_fcm::coco_fcm(coco &cc) noexcept : coco_module(cc), access_token_client("oauth2.googleapis.com", 443), client("fcm.googleapis.com", 443)
    {
        [[maybe_unused]] auto send_notification_err = AddUDF(get_env(), "send_notification", "v", 3, 3, "yss", send_notification_udf, "send_notification_udf", this);
        assert(send_notification_err == AUE_NO_ERROR);
    }

    void coco_fcm::send_notification(const std::string &token, const std::string &title, const std::string &body)
    {
        if (access_token.empty() || std::chrono::system_clock::now() >= access_token_expiration)
            refresh_access_token();
        json::json j_message{{"message", {{"token", token.c_str()}, {"notification", {{"title", title.c_str()}, {"body", body.c_str()}}}}}};
        auto res = client.post("/v1/projects/" COCO_NAME "/messages:send", std::move(j_message), {{"Content-Type", "application/json"}, {"Authorization", "Bearer " + access_token}});
    }

    void coco_fcm::refresh_access_token()
    {
        LOG_DEBUG("Refreshing FCM access token");

        json::json j_header{{"alg", "RS256"}, {"typ", "JWT"}};
        long iat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        long exp = iat + 3600; // 1 hour expiration
        json::json j_claims{{"iss", FCM_CLIENT_EMAIL}, {"scope", "https://www.googleapis.com/auth/firebase.messaging"}, {"aud", "https://oauth2.googleapis.com/token"}, {"iat", iat}, {"exp", exp}};
        auto header = utils::base64url_encode(j_header.dump());
        auto claims = utils::base64url_encode(j_claims.dump());
        auto jwt = header + "." + claims + "." + utils::base64url_encode(utils::sign_rs256(header + "." + claims, FCM_PRIVATE_KEY));
        std::string body = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt;
        auto res = access_token_client.post("/token", std::move(body), {{"Content-Type", "application/x-www-form-urlencoded"}});
        auto j_res = json::load(static_cast<network::string_response &>(*res).get_body());
        access_token = static_cast<std::string>(j_res["access_token"]);
        access_token_expiration = std::chrono::system_clock::now() + std::chrono::seconds(exp - iat);
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