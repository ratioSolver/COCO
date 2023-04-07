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
    user(const std::string &id, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &roots, const json::json &data) : id(id), first_name(first_name), last_name(last_name), email(email), password(password), roots(roots), data(data) {}

    /**
     * @brief Get the id of user.
     *
     * @return const std::string& the id of the user.
     */
    const std::string &get_id() const { return id; }
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
     * @brief Get the roots of the user.
     *
     * @return const std::vector<std::string>& The roots of the user.
     */
    const std::vector<std::string> &get_roots() const { return roots; }

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
    std::string first_name, last_name, email, password;
    std::vector<std::string> roots;
    json::json data;
    Fact *fact = nullptr;
  };

  using user_ptr = utils::u_ptr<user>;

  inline json::json to_json(const user &u)
  {
    auto res = json::json{{"id", u.get_id()},
                          {"first_name", u.get_first_name()},
                          {"last_name", u.get_last_name()},
                          {"email", u.get_email()},
                          {"data", u.get_data()}};
    auto roots = json::json(json::json_type::array);
    for (const auto &root : u.get_roots())
      roots.push_back(root);
    res["roots"] = roots;
    return res;
  }
} // namespace coco
