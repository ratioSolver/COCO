#pragma once

#include "json.h"
#include "memory.h"
#include "clips.h"

namespace coco
{
  class coco_core;
  class coco_db;

  class user
  {
    friend class coco_core;
    friend class coco_db;

  public:
    user(const std::string &id, bool admin, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &instances = {}, const json::json &data = {}) : id(id), admin(admin), first_name(first_name), last_name(last_name), email(email), password(password), instances(instances), data(data) {}

    /**
     * @brief Get the id of user.
     *
     * @return const std::string& the id of the user.
     */
    const std::string &get_id() const { return id; }
    /**
     * @brief Get the admin flag of the user.
     *
     * @return bool the admin flag of the user.
     */
    bool is_admin() const { return admin; }
    /**
     * @brief Get the first name of the user.
     *
     * @return const std::string& The first name of the user.
     */
    const std::string &get_first_name() const { return first_name; }
    /**
     * @brief Get the last name of the user.
     *
     * @return const std::string& The last name of the user.
     */
    const std::string &get_last_name() const { return last_name; }
    /**
     * @brief Get the email of the user.
     *
     * @return const std::string& The email of the user.
     */
    const std::string &get_email() const { return email; }
    /**
     * @brief Get the password of the user.
     *
     * @return const std::string& The password of the user.
     */
    const std::string &get_password() const { return password; }
    /**
     * @brief Get the instances of the user.
     *
     * @return const std::vector<std::string>& The instances of the user.
     */
    const std::vector<std::string> &get_instances() const { return instances; }

    /**
     * @brief Get the data object of the user.
     *
     * @return const json::json& The data object of the user.
     */
    const json::json &get_data() const { return data; }

    /**
     * @brief Get the fact of the user.
     *
     * @return Fact* the fact of the user.
     */
    Fact *get_fact() const { return fact; }

  private:
    const std::string id;
    bool admin;
    std::string first_name, last_name, email, password;
    std::vector<std::string> instances;
    json::json data;
    Fact *fact = nullptr;
  };

  using user_ptr = utils::u_ptr<user>;

  inline json::json to_json(const user &u)
  {
    json::json j;
    j["id"] = u.get_id();
    j["admin"] = u.is_admin();
    j["first_name"] = u.get_first_name();
    j["last_name"] = u.get_last_name();
    j["email"] = u.get_email();
    auto instances = json::json(json::json_type::array);
    for (const auto &instance : u.get_instances())
      instances.push_back(instance);
    j["instances"] = std::move(instances);
    j["data"] = u.get_data();
    return j;
  }
} // namespace coco
