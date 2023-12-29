#include "coco_core.h"
#include "logging.h"
#include "coco_executor.h"
#include "coco_db.h"
#include "coco_listener.h"
#include <iomanip>
#include <cassert>

namespace coco
{
    void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        auto &e = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue solver_type;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        auto slv = new ratio::solver();
        auto exec = new ratio::executor::executor(*slv, solver_type.lexemeValue->contents);
        auto coco_exec = new coco_executor(e, *exec);
        e.fire_new_solver(*coco_exec);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec);

        AssertString(env, std::string("(solver (solver_ptr " + std::to_string(exec_ptr) + ") (solver_type " + solver_type.lexemeValue->contents + "))").c_str());

        e.executors.emplace_back(coco_exec);

        try
        { // we adapt to a riddle script..
            exec->adapt(riddle.lexemeValue->contents);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe script: " + std::string(e.what()));
        }

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void new_solver_files(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        auto &e = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue solver_type;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;

        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
        {
            auto &file = riddle.multifieldValue->contents[i];
            if (file.header->type != STRING_TYPE)
                return;
            fs.push_back(file.lexemeValue->contents);
        }

        auto slv = new ratio::solver();
        auto exec = new ratio::executor::executor(*slv, solver_type.lexemeValue->contents);
        auto coco_exec = new coco_executor(e, *exec);
        e.fire_new_solver(*coco_exec);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec);

        AssertString(env, std::string("(solver (solver_ptr " + std::to_string(exec_ptr) + ") (solver_type " + solver_type.lexemeValue->contents + "))").c_str());

        e.executors.emplace_back(coco_exec);

        try
        { // we adapt to some riddle files..
            exec->adapt(fs);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe files: " + std::string(e.what()));
        }

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void start_execution([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Starting plan execution..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->get_executor().start_execution();
    }

    void pause_execution([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Pausing plan execution..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->get_executor().pause_execution();
    }

    void delay_task([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Delaying task..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, INTEGER_BIT, &task_id))
            return;

        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();

        auto atm = reinterpret_cast<ratio::atom *>(task_id.integerValue->contents);
        utils::rational delay;
        UDFValue delay_val;
        if (UDFNextArgument(udfc, MULTIFIELD_BIT, &delay_val))
            switch (delay_val.multifieldValue->length)
            {
            case 1:
            {
                auto &num = delay_val.multifieldValue[0].contents;
                if (num->header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num->integerValue->contents);
                break;
            }
            case 2:
            {
                auto &num = delay_val.multifieldValue[0].contents;
                if (num->header->type != INTEGER_TYPE)
                    return;
                auto &den = delay_val.multifieldValue[1].contents;
                if (den->header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num->integerValue->contents, den->integerValue->contents);
                break;
            }
            default:
                return;
            }
        else
            delay = utils::rational::ONE;

        exec->dont_start_yet({std::pair<const ratio::atom *, utils::rational>(atm, delay)});
    }

    void extend_task([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Extending task..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, INTEGER_BIT, &task_id))
            return;

        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();

        auto atm = reinterpret_cast<ratio::atom *>(task_id.integerValue->contents);
        utils::rational delay;
        UDFValue delay_val;
        if (UDFNextArgument(udfc, MULTIFIELD_BIT, &delay_val))
            switch (delay_val.multifieldValue->length)
            {
            case 1:
            {
                auto &num = delay_val.multifieldValue[0].contents;
                if (num->header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num->integerValue->contents);
                break;
            }
            case 2:
            {
                auto &num = delay_val.multifieldValue[0].contents;
                if (num->header->type != INTEGER_TYPE)
                    return;
                auto &den = delay_val.multifieldValue[1].contents;
                if (den->header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num->integerValue->contents, den->integerValue->contents);
                break;
            }
            default:
                return;
            }
        else
            delay = utils::rational::ONE;

        exec->dont_end_yet({std::pair<const ratio::atom *, utils::rational>(atm, delay)});
    }

    void failure([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Task(s) failure..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;

        UDFValue task_ids;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &task_ids))
            return;

        std::unordered_set<const ratio::atom *> atms;
        for (size_t i = 0; i < task_ids.multifieldValue->length; ++i)
        {
            auto &atm_id = task_ids.multifieldValue->contents[i];
            if (atm_id.header->type != INTEGER_TYPE)
                return;
            atms.insert(reinterpret_cast<ratio::atom *>(atm_id.integerValue->contents));
        }

        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->get_executor().failure(atms);
    }

    void adapt_script([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Adapting to RiDDLe snippet..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        try
        { // we adapt to a riddle script..
            exec->adapt(riddle.lexemeValue->contents);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe script: " + std::string(e.what()));
        }
    }

    void adapt_files([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Adapting to RiDDLe files..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
        {
            auto &file = riddle.multifieldValue->contents[i];
            if (file.header->type != STRING_TYPE)
                return;
            fs.push_back(file.lexemeValue->contents);
        }

        try
        { // we adapt to some riddle files..
            exec->adapt(fs);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe files: " + std::string(e.what()));
        }
    }

    void delete_solver([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Deleting solver..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto &e = coco_exec->get_core();
        auto exec = &coco_exec->get_executor();
        auto slv = &exec->get_solver();
        auto id = get_id(coco_exec->get_executor().get_solver());

        auto coco_exec_it = std::find_if(e.executors.cbegin(), e.executors.cend(), [coco_exec](auto &slv_ptr)
                                         { return &*slv_ptr == coco_exec; });
        assert(*coco_exec_it);

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (retract ?slv))").c_str(), NULL);
        Run(env, -1);

        e.executors.erase(coco_exec_it);
        delete exec;
        delete slv;

        e.fire_removed_solver(id);
    }

    COCO_EXPORT coco_core::coco_core(coco_db &db) : db(db), coco_timer(1000, std::bind(&coco_core::tick, this)), env(CreateEnvironment())
    {
        AddUDF(env, "new_solver_script", "l", 2, 2, "ys", new_solver_script, "new_solver_script", this);
        AddUDF(env, "new_solver_files", "l", 2, 2, "ym", new_solver_files, "new_solver_files", this);
        AddUDF(env, "start_execution", "v", 1, 1, "l", start_execution, "start_execution", this);
        AddUDF(env, "pause_execution", "v", 1, 1, "l", pause_execution, "pause_execution", this);
        AddUDF(env, "delay_task", "v", 2, 3, "llm", delay_task, "delay_task", this);
        AddUDF(env, "extend_task", "v", 2, 3, "llm", extend_task, "extend_task", this);
        AddUDF(env, "failure", "v", 2, 2, "lm", failure, "failure", this);
        AddUDF(env, "adapt_script", "v", 2, 2, "ls", adapt_script, "adapt_script", this);
        AddUDF(env, "adapt_files", "v", 2, 2, "lm", adapt_files, "adapt_files", this);
        AddUDF(env, "delete_solver", "v", 1, 1, "l", delete_solver, "delete_solver", this);
    }
    COCO_EXPORT coco_core::~coco_core()
    {
        DestroyEnvironment(env);
    }

    COCO_EXPORT void coco_core::load_rules(const std::vector<std::string> &files)
    {
        LOG("Initializing deduCtiOn and abduCtiOn (COCO) reasoner..");

        LOG_DEBUG("Loading policy rules..");
        for (const auto &f : files)
            Load(env, f.c_str());

        Reset(env);

        db.init();

        for (const auto &st : db.get_sensor_types())
        {
            LOG_DEBUG("Adding sensor type (" << st.get().id << ") '" << st.get().name << "' to the knowledge base..");
            st.get().fact = AssertString(env, ("(sensor_type (id " + st.get().id + ") (name \"" + st.get().name + "\") (description \"" + st.get().description + "\"))").c_str());
        }

        for (const auto &s : db.get_sensors())
        {
            LOG_DEBUG("Adding sensor (" << s.get().id << ") '" << s.get().name << "' of type '" << s.get().type.name << "' to the knowledge base..");
            std::string f_str = "(sensor (id " + s.get().id + ") (sensor_type " + s.get().type.id + ") (name \"" + s.get().name + "\")";
            if (s.get().loc)
                f_str += " (location " + std::to_string(s.get().loc->x) + " " + std::to_string(s.get().loc->y) + ")";
            f_str += ')';

            s.get().fact = AssertString(env, f_str.c_str());
        }

        Run(env, -1);
    }

    COCO_EXPORT void coco_core::create_instance(const std::string &name, const json::json &data)
    {
        LOG_DEBUG("Creating new instance..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        db.create_instance(name, data);
    }

    COCO_EXPORT void coco_core::create_sensor_type(const std::string &name, const std::string &description, std::vector<parameter_ptr> &&parameters)
    {
        LOG_DEBUG("Creating new sensor type..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we store the sensor type in the database..
        auto id = db.create_sensor_type(name, description, std::move(parameters));
        // we create a new fact for the new sensor type..
        db.get_sensor_type(id).fact = AssertString(env, ("(sensor_type (id " + id + ") (name \"" + name + "\") (description \"" + description + "\"))").c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_new_sensor_type(db.get_sensor_type(id));
    }
    COCO_EXPORT void coco_core::set_sensor_type_name(sensor_type &type, const std::string &name)
    {
        LOG_DEBUG("Setting sensor type name..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the sensor type in the database..
        db.set_sensor_type_name(type, name);
        // we update the sensor type fact..
        Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + type.id + ") (modify ?st (name \"" + name + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_updated_sensor_type(type);
    }
    COCO_EXPORT void coco_core::set_sensor_type_description(sensor_type &type, const std::string &description)
    {
        LOG_DEBUG("Setting sensor type description..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the sensor type in the database..
        db.set_sensor_type_description(type, description);
        // we update the sensor type fact
        Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + type.id + ") (modify ?st (description \"" + description + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_updated_sensor_type(type);
    }
    COCO_EXPORT void coco_core::delete_sensor_type(sensor_type &type)
    {
        LOG_DEBUG("Deleting sensor type..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        auto id = type.id;
        auto f = db.get_sensor_type(id).fact;
        // we delete the sensor type from the database..
        db.delete_sensor_type(type);
        // we retract the sensor type fact..
        Retract(f);
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_removed_sensor_type(id);
    }

    COCO_EXPORT void coco_core::create_sensor(const std::string &name, sensor_type &type, location_ptr l)
    {
        LOG_DEBUG("Creating new sensor " << name << " of type " << type.id << "..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we store the sensor in the database..
        auto id = db.create_sensor(name, type, std::move(l));
        // we create a new fact for the new sensor..
        std::string f_str = "(sensor (id " + id + ") (sensor_type " + type.id + ") (name \"" + name + "\")";
        if (l)
            f_str += " (location " + std::to_string(l->x) + " " + std::to_string(l->y) + ")";
        f_str += ')';
        db.get_sensor(id).fact = AssertString(env, f_str.c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_new_sensor(db.get_sensor(id));
    }
    COCO_EXPORT void coco_core::set_sensor_name(sensor &s, const std::string &name)
    {
        LOG_DEBUG("Setting sensor name..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the sensor in the database..
        db.set_sensor_name(s, name);
        // we update the sensor fact..
        Eval(env, ("(do-for-fact ((?s sensor)) (= ?s:id " + s.id + ") (modify ?s (name \"" + name + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_updated_sensor(s);
    }
    COCO_EXPORT void coco_core::set_sensor_location(sensor &s, location_ptr l)
    {
        LOG_DEBUG("Setting sensor location..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        double s_x = l->x, s_y = l->y;
        // we update the sensor in the database..
        db.set_sensor_location(s, std::move(l));
        // we update the sensor fact..
        Eval(env, ("(do-for-fact ((?s sensor_type)) (= ?s:id " + s.id + ") (modify ?s (location " + std::to_string(s_x) + " " + std::to_string(s_y) + ")))").c_str(), NULL);

        fire_updated_sensor(s);
    }
    COCO_EXPORT void coco_core::set_sensor_data(sensor &s, const json::json &value)
    {
        LOG_DEBUG("Setting sensor value..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);

        auto time = std::chrono::system_clock::now();
        std::time_t time_t = std::chrono::system_clock::to_time_t(time);
        LOG_DEBUG("Time: " << std::put_time(std::localtime(&time_t), "%c %Z"));
        LOG_DEBUG("Value: " << value.to_string());
        db.set_sensor_data(s, time, value);

        FunctionCallBuilder *sensor_data = CreateFunctionCallBuilder(env, 3);
        FCBAppendFact(sensor_data, s.fact);
        FCBAppendFact(sensor_data, s.type.fact);
        FCBAppendInteger(sensor_data, time_t);
        FCBAppendMultifield(sensor_data, StringToMultifield(env, value_to_string(value).c_str()));
        FCBCall(sensor_data, "sensor_data", NULL);
        Run(env, -1);
        FCBDispose(sensor_data);

        fire_new_sensor_data(s, time, value);
    }
    COCO_EXPORT void coco_core::set_sensor_state(sensor &s, const json::json &state)
    {
        LOG_DEBUG("Setting sensor state..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);

        auto time = std::chrono::system_clock::now();
        [[maybe_unused]] std::time_t time_t = std::chrono::system_clock::to_time_t(time);
        LOG_DEBUG("Time: " << std::put_time(std::localtime(&time_t), "%c %Z"));
        LOG_DEBUG("State: " << state.to_string());
        fire_new_sensor_state(s, time, state);

        FunctionCallBuilder *sensor_state = CreateFunctionCallBuilder(env, 3);
        FCBAppendFact(sensor_state, s.fact);
        FCBAppendFact(sensor_state, s.type.fact);
        FCBAppendInteger(sensor_state, time_t);
        FCBAppendMultifield(sensor_state, StringToMultifield(env, value_to_string(state).c_str()));
        FCBCall(sensor_state, "sensor_state", NULL);
        Run(env, -1);
        FCBDispose(sensor_state);
    }
    COCO_EXPORT void coco_core::delete_sensor(sensor &s)
    {
        LOG_DEBUG("Deleting sensor..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        auto id = s.id;
        auto f = db.get_sensor(id).fact;
        // we delete the sensor from the database..
        db.delete_sensor(s);
        // we retract the sensor fact..
        Retract(f);
        // we run the rules engine to update the policy..
        Run(env, -1);

        fire_removed_sensor(id);
    }

    void coco_core::tick()
    {
        const std::lock_guard<std::recursive_mutex> lock(mtx);

        Eval(env, "(tick)", NULL);

        std::vector<coco_executor *> c_executors;
        c_executors.reserve(executors.size());
        for (auto &exec : executors)
            c_executors.push_back(exec.operator->());
        for (auto &exec : c_executors)
            exec->tick();
    }

    void coco_core::fire_new_sensor_type(const sensor_type &st)
    {
        for (const auto &l : listeners)
            l->new_sensor_type(st);
    }
    void coco_core::fire_updated_sensor_type(const sensor_type &st)
    {
        for (const auto &l : listeners)
            l->updated_sensor_type(st);
    }
    void coco_core::fire_removed_sensor_type(const std::string &id)
    {
        for (const auto &l : listeners)
            l->removed_sensor_type(id);
    }

    void coco_core::fire_new_sensor(const sensor &s)
    {
        for (const auto &l : listeners)
            l->new_sensor(s);
    }
    void coco_core::fire_updated_sensor(const sensor &s)
    {
        for (const auto &l : listeners)
            l->updated_sensor(s);
    }
    void coco_core::fire_removed_sensor(const std::string &id)
    {
        for (const auto &l : listeners)
            l->removed_sensor(id);
    }

    void coco_core::fire_new_sensor_data(const sensor &s, const std::chrono::system_clock::time_point &time, const json::json &value)
    {
        for (const auto &l : listeners)
            l->new_sensor_data(s, time, value);
    }
    void coco_core::fire_new_sensor_state(const sensor &s, const std::chrono::system_clock::time_point &time, const json::json &state)
    {
        for (const auto &l : listeners)
            l->new_sensor_state(s, time, state);
    }

    void coco_core::fire_new_solver(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->new_solver(exec);
    }
    void coco_core::fire_removed_solver(const uintptr_t id)
    {
        for (const auto &l : listeners)
            l->removed_solver(id);
    }

    void coco_core::fire_state_changed(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->state_changed(exec);
    }

    void coco_core::fire_started_solving(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->started_solving(exec);
    }
    void coco_core::fire_solution_found(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->solution_found(exec);
    }
    void coco_core::fire_inconsistent_problem(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->inconsistent_problem(exec);
    }

    void coco_core::fire_flaw_created(const coco_executor &exec, const ratio::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_created(exec, f);
    }
    void coco_core::fire_flaw_state_changed(const coco_executor &exec, const ratio::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_state_changed(exec, f);
    }
    void coco_core::fire_flaw_cost_changed(const coco_executor &exec, const ratio::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_cost_changed(exec, f);
    }
    void coco_core::fire_flaw_position_changed(const coco_executor &exec, const ratio::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_position_changed(exec, f);
    }
    void coco_core::fire_current_flaw(const coco_executor &exec, const ratio::flaw &f)
    {
        for (const auto &l : listeners)
            l->current_flaw(exec, f);
    }

    void coco_core::fire_resolver_created(const coco_executor &exec, const ratio::resolver &r)
    {
        for (const auto &l : listeners)
            l->resolver_created(exec, r);
    }
    void coco_core::fire_resolver_state_changed(const coco_executor &exec, const ratio::resolver &r)
    {
        for (const auto &l : listeners)
            l->resolver_state_changed(exec, r);
    }
    void coco_core::fire_current_resolver(const coco_executor &exec, const ratio::resolver &r)
    {
        for (const auto &l : listeners)
            l->current_resolver(exec, r);
    }

    void coco_core::fire_causal_link_added(const coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r)
    {
        for (const auto &l : listeners)
            l->causal_link_added(exec, f, r);
    }

    void coco_core::fire_message_arrived(const std::string &topic, const json::json &msg)
    {
        for (const auto &l : listeners)
            l->message_arrived(topic, msg);
    }

    void coco_core::fire_executor_state_changed(const coco_executor &exec, ratio::executor::executor_state state)
    {
        for (const auto &l : listeners)
            l->executor_state_changed(exec, state);
    }

    void coco_core::fire_tick(const coco_executor &exec, const utils::rational &time)
    {
        for (const auto &l : listeners)
            l->tick(exec, time);
    }

    void coco_core::fire_start(const coco_executor &exec, const std::unordered_set<ratio::atom *> &atoms)
    {
        for (const auto &l : listeners)
            l->start(exec, atoms);
    }
    void coco_core::fire_end(const coco_executor &exec, const std::unordered_set<ratio::atom *> &atoms)
    {
        for (const auto &l : listeners)
            l->end(exec, atoms);
    }

    json::json solvers_message(const std::list<coco_executor_ptr> &executors) noexcept
    {
        json::json j_msg{{"type", "solvers"}};
        json::json j_solvers = json::json(json::json_type::array);
        for (const auto &exec : executors)
            j_solvers.push_back({{"id", get_id(exec->get_executor().get_solver())}, {"name", exec->get_executor().get_name()}, {"state", to_string(exec->get_executor().get_state())}});
        j_msg["solvers"] = j_solvers;
        return j_msg;
    }
} // namespace coco
