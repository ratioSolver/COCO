#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"
#include "client.hpp"
#include <chrono>

namespace coco
{
  class coco_fcm final : public coco_module
  {
  public:
    coco_fcm(coco &cc, std::string_view fcm_project_id = std::getenv("FCM_PROJECT_ID"), std::string_view client_email = std::getenv("FCM_CLIENT_EMAIL"), std::string_view private_key = std::getenv("FCM_PRIVATE_KEY")) noexcept;

    void add_token(std::string_view id, std::string_view token);
    void send_notification(std::string_view id, std::string_view title, std::string_view body);

  private:
    friend void send_notification_udf(Environment *env, UDFContext *udfc, UDFValue *out);

    void refresh_access_token();

  private:
    const std::string fcm_project_id;                                           // FCM Project ID
    const std::string client_email, private_key;                                // FCM service account credentials
    network::ssl_client access_token_client;                                    // Client for obtaining access tokens
    std::string access_token;                                                   // Access token for FCM
    std::chrono::time_point<std::chrono::system_clock> access_token_expiration; // Expiration time of the access token
    network::ssl_client client;                                                 // FCM client for push notifications
  };

  void send_notification_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco
