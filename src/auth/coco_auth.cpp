#include "coco_auth.hpp"
#include "coco.hpp"
#include "auth_db.hpp"

namespace coco
{
    authentication::authentication(coco &cc) noexcept : coco_module(cc)
    {
        try
        {
            [[maybe_unused]] auto &tp = cc.get_type(user_kw);
        }
        catch (const std::invalid_argument &)
        { // Type does not exist, create it
            [[maybe_unused]] auto &tp = cc.create_type(user_kw, {}, {}, {}, {});
        }
        cc.get_db().add_module<auth_db>(static_cast<mongo_db &>(cc.get_db()));
    }

    std::string authentication::get_token(std::string_view username, std::string_view password)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto user = cc.get_db().get_module<auth_db>().get_user(username, password);
        return user.id;
    }

    item &authentication::create_user(std::string_view username, std::string_view password, json::json &&personal_data)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto &tp = cc.get_type(user_kw);
        auto &itm = cc.create_item(tp);
        cc.get_db().get_module<auth_db>().create_user(itm.get_id(), username, password, std::move(personal_data));
        return itm;
    }
} // namespace coco
