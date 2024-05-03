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
    sensor &coco_core::create_sensor(const sensor_type &type, const std::string &name, json::json &&data)
    {
        auto &s = db.create_sensor(type, name, std::move(data));
        new_sensor(s);
        return s;
    }

    coco_executor &coco_core::create_solver(const std::string &name)
    {
        auto slv = std::make_unique<coco_executor>(*this, std::make_shared<ratio::solver>(name));
        auto &slv_ref = *slv;
        new_solver(*executors.insert(std::move(slv)).first->get());
        return slv_ref;
    }

    void coco_core::new_sensor_type([[maybe_unused]] const sensor_type &s) { LOG_TRACE("New sensor type: " + s.get_id()); }
    void coco_core::updated_sensor_type([[maybe_unused]] const sensor_type &s) { LOG_TRACE("Updated sensor type: " + s.get_id()); }
    void coco_core::deleted_sensor_type([[maybe_unused]] const std::string &id) { LOG_TRACE("Deleted sensor type: " + id); }

    void coco_core::new_sensor([[maybe_unused]] const sensor &s) { LOG_TRACE("New sensor: " + s.get_id()); }
    void coco_core::updated_sensor([[maybe_unused]] const sensor &s) { LOG_TRACE("Updated sensor: " + s.get_id()); }
    void coco_core::deleted_sensor([[maybe_unused]] const std::string &id) { LOG_TRACE("Deleted sensor: " + id); }

    void coco_core::new_sensor_value([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &value) { LOG_TRACE("Sensor " + s.get_id() + " value: " + value.to_string()); }
    void coco_core::new_sensor_state([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &state) { LOG_TRACE("Sensor " + s.get_id() + " state: " + state.to_string()); }

    void coco_core::new_solver([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("New solver: " + exec.get_solver().get_name() + " (" + std::to_string(get_id(exec)) + ")"); }
    void coco_core::deleted_solver([[maybe_unused]] const uintptr_t id) { LOG_TRACE("Deleted solver: " + std::to_string(id)); }

    void coco_core::state_changed([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " is now " + to_string(exec.get_state())); }

    void coco_core::started_solving([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " started solving"); }
    void coco_core::solution_found([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " found a solution"); }
    void coco_core::inconsistent_problem([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " found an inconsistent problem"); }

    void coco_core::flaw_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " found a flaw: " + to_string(f.get_phi())); }
    void coco_core::flaw_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Flaw " + to_string(f.get_phi()) + " is now " + to_state(f)); }
    void coco_core::flaw_cost_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Flaw " + to_string(f.get_phi()) + " cost changed to " + to_string(f.get_estimated_cost())); }
    void coco_core::flaw_position_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Flaw " + to_string(f.get_phi()) + " position changed to " + std::to_string(exec.get_solver().get_idl_theory().bounds(f.get_position()).first)); }
    void coco_core::current_flaw([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Current flaw: " + to_string(f.get_phi())); }

    void coco_core::resolver_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " created a resolver: " + to_string(r.get_rho())); }
    void coco_core::resolver_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Resolver " + to_string(r.get_rho()) + " is now " + to_state(r)); }
    void coco_core::current_resolver([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Current resolver: " + to_string(r.get_rho())); }

    void coco_core::causal_link_added([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Causal link added: " + to_string(f.get_phi()) + " -> " + to_string(r.get_rho())); }

    void coco_core::executor_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] ratio::executor::executor_state state) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " is now " + to_string(state)); }

    void coco_core::tick([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const utils::rational &time) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " ticked at " + time.to_string()); }

    void coco_core::start([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::unordered_set<ratio::atom *> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " started atoms: ";
        for (const auto &a : atoms)
            str += to_string(get_id(*a)) + " ";
        LOG_TRACE(str);
#endif
    }
    void coco_core::end([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::unordered_set<ratio::atom *> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " ended atoms: ";
        for (const auto &a : atoms)
            str += to_string(get_id(*a)) + " ";
        LOG_TRACE(str);
#endif
    }
} // namespace coco