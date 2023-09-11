#pragma once

#include "json.h"
#include "memory.h"

namespace coco
{
  class instance
  {
  public:
    instance(const std::string &id, const std::string &name, const std::vector<std::string> &users = {}, const json::json &data = {}) : id(id), name(name), users(users), data(data) {}

    /**
     * @brief Get the id of instance.
     *
     * @return const std::string& the id of the instance.
     */
    const std::string &get_id() const { return id; }

    /**
     * @brief Get the name of instance.
     *
     * @return const std::string& the name of the instance.
     */
    const std::string &get_name() const { return name; }

    /**
     * @brief Get the users of instance.
     *
     * @return const std::vector<std::string>& the users of the instance.
     */
    const std::vector<std::string> &get_users() const { return users; }

    /**
     * @brief Get the data object of the instance.
     *
     * @return const json::json& The data object of the instance.
     */
    const json::json &get_data() const { return data; }

  private:
    std::string id;
    std::string name;
    std::vector<std::string> users;
    json::json data;
  };

  using instance_ptr = utils::u_ptr<instance>;

  inline json::json to_json(const instance &u)
  {
    json::json j;
    j["id"] = u.get_id();
    j["name"] = u.get_name();
    json::json users(json::json_type::array);
    for (const auto &user : u.get_users())
      users.push_back(user);
    j["users"] = users;
    j["data"] = u.get_data();
    return j;
  }
} // namespace coco
