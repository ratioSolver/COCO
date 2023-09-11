#pragma once

#include "instance.h"
#include "sensor_type.h"
#include "sensor.h"
#include "user.h"
#include <unordered_map>
#include <functional>

namespace coco
{
  class coco_db
  {
  public:
    coco_db(const std::string &instance = COCO_INSTANCE, const std::string &app = COCO_APP);
    virtual ~coco_db() = default;

    const std::string &get_app() const { return app; }
    const std::string &get_instance() const { return inst; }

    virtual void init();

    virtual std::string create_instance(const std::string &name, const json::json &data = {}) = 0;

    /**
     * @brief Returns all the instances of the app.
     *
     * @return std::vector<instance> the instances of the app.
     */
    virtual std::vector<instance_ptr> get_instances() = 0;

    /**
     * @brief Create a user object with the given first name, last name, email, password and type and returns its id.
     *
     * @param admin The admin flag of the user.
     * @param first_name The first name of the user.
     * @param last_name The last name of the user.
     * @param email The email of the user.
     * @param password The password of the user.
     * @param data The data of the user.
     * @return std::string The id of the created user.
     */
    virtual std::string create_user(bool admin, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &instances = {}, const json::json &data = {});

    /**
     * @brief Check if the user object with the given id exists.
     *
     * @param id the id of the user.
     * @return true if the user exists.
     * @return false if the user does not exist.
     */
    virtual bool has_user(const std::string &id) const { return users.find(id) != users.end(); }
    /**
     * @brief Get the user object with the given id.
     *
     * @param id the id of the user.
     * @return user_ptr the user with the given id.
     */
    virtual user &get_user(const std::string &id) const { return *users.at(id); }
    /**
     * @brief Get the users object.
     *
     * @return std::vector<std::reference_wrapper<user>> the users.
     */
    std::vector<std::reference_wrapper<user>> get_users();
    /**
     * @brief Get the user object with the given email and password.
     *
     * @param email the email of the user.
     * @param password the password of the user.
     * @return user_ptr the user with the given email and password.
     */
    virtual user_ptr get_user(const std::string &email, const std::string &password) = 0;

    /**
     * @brief Get the all the users.
     *
     * @return std::vector<user_ptr> the users.
     */
    virtual std::vector<user_ptr> get_all_users() = 0;

    /**
     * @brief Set the user's first name.
     *
     * @param u the user to update.
     * @param first_name the new first name of the user.
     */
    virtual void set_user_first_name(user &u, const std::string &first_name) { u.first_name = first_name; }
    virtual void set_user_first_name(const std::string &id, const std::string &first_name) { set_user_first_name(*users.at(id), first_name); }
    /**
     * @brief Set the user's last name.
     *
     * @param u the user to update.
     * @param last_name the new last name of the user.
     */
    virtual void set_user_last_name(user &u, const std::string &last_name) { u.last_name = last_name; }
    virtual void set_user_last_name(const std::string &id, const std::string &last_name) { set_user_last_name(*users.at(id), last_name); }
    /**
     * @brief Set the user's email.
     *
     * @param u the user to update.
     * @param email the new email of the user.
     */
    virtual void set_user_email(user &u, const std::string &email) { u.email = email; }
    virtual void set_user_email(const std::string &id, const std::string &email) { set_user_email(*users.at(id), email); }
    /**
     * @brief Set the user's password.
     *
     * @param u the user to update.
     * @param password the new password of the user.
     */
    virtual void set_user_password(user &u, const std::string &password) { u.password = password; }
    virtual void set_user_password(const std::string &id, const std::string &password) { set_user_password(*users.at(id), password); }
    /**
     * @brief Set the user's instances.
     *
     * @param u the user to update.
     * @param instances the new instances of the user.
     */
    virtual void set_user_instances(user &u, const std::vector<std::string> &instances) { u.instances = instances; }
    virtual void set_user_instances(const std::string &id, const std::vector<std::string> &instances) { set_user_instances(*users.at(id), instances); }
    /**
     * @brief Set the user's data.
     *
     * @param u the user to update.
     * @param data the new data of the user.
     */
    virtual void set_user_data(user &u, const json::json &data) { u.data = data; }
    virtual void set_user_data(const std::string &id, const json::json &data) { set_user_data(*users.at(id), data); }

    /**
     * @brief Delete the user object with the given id.
     *
     * @param u the user to delete.
     */
    virtual void delete_user(user &u) { users.erase(u.id); }
    virtual void delete_user(const std::string &id) { delete_user(*users.at(id)); }

    /**
     * @brief Create a sensor type object with the given name and description and returns its id.
     *
     * @param name the name of the sensor type
     * @param description the description of the sensor type
     * @param parameter_types the parameter types of the sensors of the sensor type
     * @return std::string the id of the created sensor type
     */
    virtual std::string create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types);
    /**
     * @brief Get the all sensor types object.
     *
     * @return std::vector<std::reference_wrapper<sensor_type>>
     */
    std::vector<std::reference_wrapper<sensor_type>> get_sensor_types();
    /**
     * @brief Check if the sensor type object with the given id exists.
     *
     * @param id the id of the sensor type.
     * @return true if the sensor type exists.
     * @return false if the sensor type does not exist.
     */
    bool has_sensor_type(const std::string &id) const { return sensor_types.find(id) != sensor_types.end(); }
    /**
     * @brief Get the sensor type object with the given id.
     *
     * @param id the id of the sensor type.
     * @return sensor_type& the sensor type with the given id.
     */
    sensor_type &get_sensor_type(const std::string &id) { return *sensor_types.at(id); }
    /**
     * @brief Get the sensor type object with the given name.
     *
     * @param name the name of the sensor type.
     * @return sensor_type& the sensor type with the given name.
     */
    sensor_type &get_sensor_type_by_name(const std::string &name) { return sensor_types_by_name.at(name).get(); }
    /**
     * @brief Set the name to the sensor type object with the given id.
     *
     * @param st the sensor type.
     * @param name the new name of the sensor type.
     */
    virtual void set_sensor_type_name(sensor_type &st, const std::string &name) { st.name = name; }
    /**
     * @brief Set the description to the sensor type object with the given id.
     *
     * @param st the sensor type.
     * @param description the new description of the sensor type.
     */
    virtual void set_sensor_type_description(sensor_type &st, const std::string &description) { st.description = description; }
    /**
     * @brief Delete the sensor type object with the given id.
     *
     * @param st the sensor type.
     */
    virtual void delete_sensor_type(sensor_type &st) { sensor_types.erase(st.id); }

    /**
     * @brief Create a sensor object with the given name, type and location and returns its id.
     *
     * @param name the name of the sensor.
     * @param type the type of the sensor.
     * @param l the location of the sensor.
     * @return std::string the id of the created sensor.
     */
    virtual std::string create_sensor(const std::string &name, sensor_type &type, location_ptr l = nullptr);
    /**
     * @brief Get all the sensors object.
     *
     * @return std::vector<std::reference_wrapper<sensor>> all the sensors.
     */
    std::vector<std::reference_wrapper<sensor>> get_sensors();
    /**
     * @brief Check if the sensor object with the given id exists.
     *
     * @param id the id of the sensor.
     * @return true if the sensor exists.
     * @return false if the sensor does not exist.
     */
    bool has_sensor(const std::string &id) const { return sensors.find(id) != sensors.end(); }
    /**
     * @brief Get the sensor object with the given id.
     *
     * @param id the id of the sensor.
     * @return sensor& the sensor with the given id.
     */
    sensor &get_sensor(const std::string &id) { return *sensors.at(id); }
    /**
     * @brief Set the name to the sensor object with the given id.
     *
     * @param s the sensor.
     * @param name the new name of the sensor.
     */
    virtual void set_sensor_name(sensor &s, const std::string &name) { s.name = name; }
    /**
     * @brief Set the type to the sensor object with the given id.
     *
     * @param s the sensor.
     * @param type the new type of the sensor.
     */
    virtual void set_sensor_location(sensor &s, location_ptr l) { s.set_location(std::move(l)); }
    /**
     * @brief Get the last sensor value of the sensor object with the given id.
     *
     * @param s the sensor.
     * @return json::json the last sensor value of the sensor with the given id.
     */
    virtual json::json get_last_sensor_value(sensor &s);
    /**
     * @brief Get the sensor values of the sensor object from the given start time to the given end time.
     *
     * @param s the sensor.
     * @param from the start time of the sensor values.
     * @param to the end time of the sensor values.
     * @return json::json the sensor values of the sensor with the given id.
     */
    virtual json::json get_sensor_values(sensor &s, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to);
    /**
     * @brief Set the value of the sensor object with the given id.
     *
     * @param s the sensor.
     * @param time the time of the sensor value.
     * @param val the value of the sensor.
     */
    virtual void set_sensor_value(sensor &s, const std::chrono::system_clock::time_point &time, const json::json &val) { s.set_value(time, val); }
    /**
     * @brief Delete the sensor object with the given id.
     *
     * @param s the sensor.
     */
    virtual void delete_sensor(sensor &s);

    /**
     * @brief Drop the database.
     *
     */
    virtual void drop();

  protected:
    user &create_user(const std::string &id, bool admin, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &instances = {}, const json::json &data = {});
    sensor_type &create_sensor_type(const std::string &id, const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types);
    sensor &create_sensor(const std::string &id, const std::string &name, sensor_type &type, location_ptr l);

  private:
    const std::string app;                                                                     // The app name.
    const std::string inst;                                                                    // The instance name.
    std::unordered_map<std::string, user_ptr> users;                                           // The users of the current instance.
    std::unordered_map<std::string, sensor_type_ptr> sensor_types;                             // The sensor types of the app.
    std::unordered_map<std::string, std::reference_wrapper<sensor_type>> sensor_types_by_name; // The sensor types of the app indexed by name.
    std::unordered_map<std::string, sensor_ptr> sensors;                                       // The sensors of the current instance.
  };
} // namespace coco
