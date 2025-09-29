#pragma once

#include "coco_server.hpp"
#include "coco_module.hpp"
#include "coco_item.hpp"

namespace coco
{
  constexpr const char *user_kw = "User";

  class coco_auth : public coco_module
  {
  public:
    coco_auth(coco &cc) noexcept;

    /**
     * @brief Checks whether the provided authentication token is valid.
     *
     * This function verifies the validity of the given token string.
     *
     * @param token The authentication token to validate.
     * @return true if the token is valid, false otherwise.
     */
    [[nodiscard]] bool is_valid_token(std::string_view token) const noexcept;
    /**
     * @brief Retrieves an authentication token for the specified user credentials.
     *
     * This function generates and returns an authentication token based on the provided
     * username and password. The returned token can be used for subsequent authenticated
     * requests.
     *
     * @param username The username to authenticate.
     * @param password The password associated with the username.
     * @return std::string The generated authentication token.
     */
    [[nodiscard]] std::string get_token(std::string_view username, std::string_view password);

    /**
     * @brief Retrieves a vector of references to all users.
     *
     * This function retrieves all users stored in the database and returns them as a vector of `item` objects.
     * The returned vector contains references to the actual user items stored in the database.
     *
     * @return A vector of user items.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<item>> get_users() noexcept;
    /**
     * @brief Creates a new user with the specified username and password.
     *
     * This function creates a new user item in the database with the provided username and password.
     * Optionally, personal data can be provided as a JSON object.
     *
     * @param username The username for the new user.
     * @param password The password for the new user.
     * @param user_role The role of the user (default is `1`).
     * @param personal_data Optional personal data for the user in JSON format.
     * @return item& A reference to the created user item.
     */
    [[nodiscard]] item &create_user(std::string_view username, std::string_view password, int8_t user_role = 1, json::json &&personal_data = {});
  };

  class server_auth : public server_module
  {
  public:
    server_auth(coco_server &srv) noexcept;

  private:
    std::unique_ptr<network::response> login(const network::request &req);
    std::unique_ptr<network::response> get_users(const network::request &req);
    std::unique_ptr<network::response> create_user(const network::request &req);

  private:
    void on_ws_open(network::ws_server_session_base &ws) override;
    void on_ws_message(network::ws_server_session_base &ws, const network::message &msg) override;
    void on_ws_close(network::ws_server_session_base &ws) override;
    void on_ws_error(network::ws_server_session_base &ws, const std::error_code &) override;

    void broadcast(json::json &msg) override;

  private:
    std::mutex mtx;
    std::unordered_map<network::ws_server_session_base *, std::string> clients;
    std::unordered_map<std::string, std::set<network::ws_server_session_base *>> devices;
  };

  class auth_middleware : public network::middleware
  {
  public:
    auth_middleware(coco_server &srv, coco &cc, std::map<network::verb, std::vector<std::string>> &&excluded_paths = {{network::Get, {"^/$", "^/assets/.*", "/.+\\.ico", "/.+\\.png", "/.+\\.jpg", "^/openapi$", "^/asyncapi$"}}, {network::Post, {"^/login$"}}}) noexcept;

    void add_excluded_path(network::verb v, std::string_view pattern);
    void add_authorized_path(network::verb v, std::string_view pattern, std::set<uint8_t> roles, bool self = false);

  private:
    std::unique_ptr<network::response> before_request(const network::request &req) override;

  private:
    coco_server &srv;                                                                                        // Reference to the coco_server instance
    coco &cc;                                                                                                // Reference to the coco instance
    std::map<network::verb, std::vector<std::string>> excluded_paths;                                        // Paths to exclude from authentication
    std::map<network::verb, std::vector<std::tuple<std::string, std::set<uint8_t>, bool>>> authorized_paths; // Paths requiring authentication with roles and self-access flag
  };
} // namespace coco
