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

    void coco_core::new_sensor_type(const sensor_type &s) { LOG_TRACE("New sensor type: " + s.get_id()); }
    void coco_core::updated_sensor_type(const sensor_type &s) { LOG_TRACE("Updated sensor type: " + s.get_id()); }
    void coco_core::deleted_sensor_type(const std::string &id) { LOG_TRACE("Deleted sensor type: " + id); }

    void coco_core::new_sensor(const sensor &s) { LOG_TRACE("New sensor: " + s.get_id()); }
    void coco_core::updated_sensor(const sensor &s) { LOG_TRACE("Updated sensor: " + s.get_id()); }
    void coco_core::deleted_sensor(const std::string &id) { LOG_TRACE("Deleted sensor: " + id); }

    void coco_core::new_sensor_value(const sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &value) { LOG_TRACE("Sensor " + s.get_id() + " value: " + value.to_string()); }
    void coco_core::new_sensor_state(const sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &state) { LOG_TRACE("Sensor " + s.get_id() + " state: " + state.to_string()); }
} // namespace coco