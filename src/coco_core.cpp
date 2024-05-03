#include <cassert>
#include "coco_core.hpp"
#include "coco_db.hpp"
#include "logging.hpp"

namespace coco
{
    coco_core::coco_core(coco_db &db) : db(db), env(CreateEnvironment())
    {
        assert(env != nullptr);
    }

    sensor_type &coco_core::create_sensor_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars)
    {
        auto &st = db.create_sensor_type(name, description, std::move(pars));
        new_sensor_type(st);
        return st;
    }

    void coco_core::new_sensor_type([[maybe_unused]] const sensor_type &s) { LOG_TRACE("New sensor type: " + s.get_id()); }
    void coco_core::updated_sensor_type([[maybe_unused]] const sensor_type &s) { LOG_TRACE("Updated sensor type: " + s.get_id()); }
    void coco_core::deleted_sensor_type([[maybe_unused]] const std::string &id) { LOG_TRACE("Deleted sensor type: " + id); }

    void coco_core::new_sensor([[maybe_unused]] const sensor &s) { LOG_TRACE("New sensor: " + s.get_id()); }
    void coco_core::updated_sensor([[maybe_unused]] const sensor &s) { LOG_TRACE("Updated sensor: " + s.get_id()); }
    void coco_core::deleted_sensor([[maybe_unused]] const std::string &id) { LOG_TRACE("Deleted sensor: " + id); }

    void coco_core::new_sensor_value([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &value) { LOG_TRACE("Sensor " + s.get_id() + " value: " + value.to_string()); }
    void coco_core::new_sensor_state([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &state) { LOG_TRACE("Sensor " + s.get_id() + " state: " + state.to_string()); }
} // namespace coco