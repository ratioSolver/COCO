#include "coco_fcm.hpp"
#include "logging.hpp"

namespace coco
{
    coco_fcm::coco_fcm(coco &cc) noexcept : coco_module(cc), client("fcm.googleapis.com", 443)
    {
        [[maybe_unused]] auto send_notification_err = AddUDF(get_env(), "send_notification", "v", 3, 3, "yss", send_notification_udf, "send_notification_udf", this);
        assert(send_notification_err == AUE_NO_ERROR);
    }

    void coco_fcm::send_notification(const std::string &token, const std::string &title, const std::string &body)
    {
        json::json j_message{{"message", {{"token", token.c_str()}, {"notification", {{"title", title.c_str()}, {"body", body.c_str()}}}}}};
        auto res = client.post("/v1/projects/" COCO_NAME "/messages:send", std::move(j_message), {{"Content-Type", "application/json"}, {"Authorization", "Bearer "}});
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