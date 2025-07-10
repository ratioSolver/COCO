#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"
#include "client.hpp"

namespace coco
{
  class coco_fcm final : public coco_module
  {
  public:
    coco_fcm(coco &cc) noexcept;

    void send_notification(const std::string &token, const std::string &title, const std::string &body);

  private:
    friend void send_notification_udf(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    network::client client; // FCM client for push notifications
  };

  void send_notification_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco
