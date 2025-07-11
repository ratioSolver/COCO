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
    coco_fcm(coco &cc) noexcept;

    void add_token(std::string_view id, std::string_view token);
    void send_notification(std::string_view id, std::string_view title, std::string_view body);

  private:
    friend void send_notification_udf(Environment *env, UDFContext *udfc, UDFValue *out);

    void refresh_access_token();

  private:
    network::ssl_client access_token_client;                                    // Client for obtaining access tokens
    std::string access_token;                                                   // Access token for FCM
    std::chrono::time_point<std::chrono::system_clock> access_token_expiration; // Expiration time of the access token
    network::ssl_client client;                                                 // FCM client for push notifications
  };

  void send_notification_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco
