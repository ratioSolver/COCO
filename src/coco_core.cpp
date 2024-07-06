#include <cassert>
#include "coco_core.hpp"
#include "coco_db.hpp"
#include "logging.hpp"

namespace coco
{
    coco_core::coco_core(std::unique_ptr<coco_db> &&db) : db(std::move(db)), env(CreateEnvironment())
    {
        assert(env != nullptr);
    }

    std::vector<std::reference_wrapper<parameter>> coco_core::get_parameters()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_parameters();
    }

    parameter &coco_core::get_parameter(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_parameter(id);
    }

    parameter &coco_core::create_parameter(const std::string &name, const std::string &description, json::json &&schema)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &par = db->create_parameter(name, description, std::move(schema));
        new_parameter(par);
        return par;
    }

    void coco_core::delete_parameter(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_parameter(db->get_parameter(id));
    }

    std::vector<std::reference_wrapper<type>> coco_core::get_types()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_types();
    }

    type &coco_core::get_type(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_type(id);
    }

    type &coco_core::create_type(const std::string &name, const std::string &description, std::map<std::string, std::reference_wrapper<parameter>> &&static_pars, std::map<std::string, std::reference_wrapper<parameter>> &&dynamic_pars)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &st = db->create_type(name, description, std::move(static_pars), std::move(dynamic_pars));
        new_type(st);
        return st;
    }

    void coco_core::delete_type(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_type(db->get_type(id));
    }

    std::vector<std::reference_wrapper<item>> coco_core::get_items()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_items();
    }

    item &coco_core::get_item(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_item(id);
    }

    item &coco_core::create_item(const type &type, const std::string &name, const json::json &pars)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &p : type.get_static_parameters())
            if (!pars.contains(p.first))
            {
                LOG_WARN("Parameters for item " + name + " do not contain parameter " + p.first);
                return db->get_items().front().get();
            }
            else if (!json::validate(pars[p.first], p.second.get().get_schema(), schemas))
            {
                LOG_WARN("Parameters for item " + name + " parameter " + p.first + " is invalid");
                return db->get_items().front().get();
            }

        auto &s = db->create_item(type, name, pars);
        new_item(s);
        return s;
    }

    void coco_core::delete_item(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_item(db->get_item(id));
    }

    void coco_core::add_data(const item &s, const json::json &data)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &p : s.get_type().get_dynamic_parameters())
            if (!data.contains(p.first))
            {
                LOG_WARN("Data for item " + s.get_id() + " does not contain parameter " + p.first);
                return;
            }
            else if (!json::validate(data[p.first], p.second.get().get_schema(), schemas))
            {
                LOG_WARN("Data for item " + s.get_id() + " parameter " + p.first + " is invalid");
                return;
            }

        db->add_data(s, std::chrono::system_clock::now(), data);
        new_data(s, std::chrono::system_clock::now(), data);
    }

    std::vector<std::reference_wrapper<coco_executor>> coco_core::get_solvers()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<coco_executor>> res;
        for (auto &exec : executors)
            res.push_back(*exec);
        return res;
    }

    coco_executor &coco_core::create_solver(const std::string &name, const utils::rational &units_per_tick)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &exec_ref = *executors.insert(std::make_unique<coco_executor>(*this, name, units_per_tick)).first->get();
        new_solver(exec_ref);
        exec_ref.init();
        return exec_ref;
    }

    std::vector<std::reference_wrapper<rule>> coco_core::get_reactive_rules()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_reactive_rules();
    }

    rule &coco_core::create_reactive_rule(const std::string &name, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &r = db->create_reactive_rule(name, content);
        new_reactive_rule(r);
        return r;
    }

    std::vector<std::reference_wrapper<rule>> coco_core::get_deliberative_rules()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_deliberative_rules();
    }

    rule &coco_core::create_deliberative_rule(const std::string &name, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &r = db->create_deliberative_rule(name, content);
        new_deliberative_rule(r);
        return r;
    }

    void coco_core::new_parameter([[maybe_unused]] const parameter &par) { LOG_TRACE("New parameter: " + par.get_id()); }
    void coco_core::updated_parameter([[maybe_unused]] const parameter &par) { LOG_TRACE("Updated parameter: " + par.get_id()); }
    void coco_core::deleted_parameter([[maybe_unused]] const std::string &par_id) { LOG_TRACE("Deleted parameter: " + par_id); }

    void coco_core::new_type([[maybe_unused]] const type &tp) { LOG_TRACE("New type: " + tp.get_id()); }
    void coco_core::updated_type([[maybe_unused]] const type &tp) { LOG_TRACE("Updated type: " + tp.get_id()); }
    void coco_core::deleted_type([[maybe_unused]] const std::string &tp_id) { LOG_TRACE("Deleted type: " + tp_id); }

    void coco_core::new_item([[maybe_unused]] const item &itm) { LOG_TRACE("New item: " + itm.get_id()); }
    void coco_core::updated_item([[maybe_unused]] const item &itm) { LOG_TRACE("Updated item: " + itm.get_id()); }
    void coco_core::deleted_item([[maybe_unused]] const std::string &itm_id) { LOG_TRACE("Deleted item: " + itm_id); }

    void coco_core::new_data([[maybe_unused]] const item &itm, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &data) { LOG_TRACE("Item " + itm.get_id() + " data: " + data.dump()); }

    void coco_core::new_solver([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("New solver: " + exec.get_solver().get_name() + " (" + std::to_string(get_id(exec)) + ")"); }
    void coco_core::deleted_solver([[maybe_unused]] const uintptr_t id) { LOG_TRACE("Deleted solver: " + std::to_string(id)); }

    void coco_core::new_reactive_rule([[maybe_unused]] const rule &r) { LOG_TRACE("New reactive rule: " + r.get_id()); }
    void coco_core::new_deliberative_rule([[maybe_unused]] const rule &r) { LOG_TRACE("New deliberative rule: " + r.get_id()); }

    void coco_core::state_changed([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " is now " + to_string(exec.get_state())); }

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

    void coco_core::tick([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const utils::rational &time) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " ticked at " + to_string(time)); }

    void coco_core::starting([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " starting atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::start([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " started atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::ending([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " ending atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::end([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " ended atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }
} // namespace coco