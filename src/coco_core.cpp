#include "coco_core.h"
#include "logging.h"
#include "coco_middleware.h"
#include "coco_executor.h"
#include "coco_db.h"
#include "coco_listener.h"

namespace coco
{
    void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue solver_type;
        if (!UDFNextArgument(udfc, LEXEME_BITS, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv);
        auto coco_exec = std::make_unique<coco_executor>(e, *exec, solver_type.lexemeValue->contents);
        e.fire_new_solver(*coco_exec);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec.get());

        AssertString(env, std::string("(solver (solver_ptr " + std::to_string(exec_ptr) + ") (solver_type " + solver_type.lexemeValue->contents + ") (state reasoning))").c_str());

        e.executors.push_back(std::move(coco_exec));

        // we adapt to a riddle script..
        exec->adapt(riddle.lexemeValue->contents);

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void new_solver_files(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue solver_type;
        if (!UDFFirstArgument(udfc, LEXEME_BITS, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv);
        auto coco_exec = std::make_unique<coco_executor>(e, *exec, solver_type.lexemeValue->contents);
        e.fire_new_solver(*coco_exec);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec.get());

        AssertString(env, std::string("(solver (solver_ptr " + std::to_string(exec_ptr) + ") (solver_type " + solver_type.lexemeValue->contents + ") (state reasoning))").c_str());

        e.executors.push_back(std::move(coco_exec));

        // we adapt to some riddle files..
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
            fs.push_back(riddle.multifieldValue->contents[i].lexemeValue->contents);
        exec->adapt(fs);

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void start_execution(Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Starting plan execution..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->start_execution();

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state executing)))").c_str(), NULL);

        e.fire_start_execution(*coco_exec);
    }

    void pause_execution(Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Pausing plan execution..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->pause_execution();

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state paused)))").c_str(), NULL);

        e.fire_pause_execution(*coco_exec);
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

        auto atm = reinterpret_cast<ratio::core::atom *>(task_id.integerValue->contents);
        semitone::rational delay;
        UDFValue delay_val;
        if (UDFNextArgument(udfc, MULTIFIELD_BIT, &delay_val))
            switch (delay_val.multifieldValue->length)
            {
            case 1:
                delay = semitone::rational(delay_val.multifieldValue[0].contents->integerValue->contents);
                break;
            case 2:
                delay = semitone::rational(delay_val.multifieldValue[0].contents->integerValue->contents, delay_val.multifieldValue[1].contents->integerValue->contents);
                break;
            }
        else
            delay = semitone::rational(1);

        exec->dont_start_yet({std::pair<const ratio::core::atom *, semitone::rational>(atm, delay)});
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

        auto atm = reinterpret_cast<ratio::core::atom *>(task_id.integerValue->contents);
        semitone::rational delay;
        UDFValue delay_val;
        if (UDFNextArgument(udfc, MULTIFIELD_BIT, &delay_val))
            switch (delay_val.multifieldValue->length)
            {
            case 1:
                delay = semitone::rational(delay_val.multifieldValue[0].contents->integerValue->contents);
                break;
            case 2:
                delay = semitone::rational(delay_val.multifieldValue[0].contents->integerValue->contents, delay_val.multifieldValue[1].contents->integerValue->contents);
                break;
            }
        else
            delay = semitone::rational(1);

        exec->dont_end_yet({std::pair<const ratio::core::atom *, semitone::rational>(atm, delay)});
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

        std::unordered_set<const ratio::core::atom *> atms;
        for (size_t i = 0; i < task_ids.multifieldValue->length; ++i)
            atms.insert(reinterpret_cast<ratio::core::atom *>(task_ids.multifieldValue->contents[i].integerValue->contents));

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

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state adapting)))").c_str(), NULL);

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        // we adapt to a riddle script..
        exec->adapt(riddle.lexemeValue->contents);
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

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state adapting)))").c_str(), NULL);

        // we adapt to some riddle files..
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
            fs.push_back(riddle.multifieldValue->contents[i].lexemeValue->contents);
        exec->adapt(fs);
    }

    void delete_solver([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Deleting solver..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();
        auto slv = &exec->get_solver();

        auto coco_exec_it = std::find_if(e.executors.cbegin(), e.executors.cend(), [coco_exec](auto &slv_ptr)
                                         { return slv_ptr.get() == coco_exec; });
        e.executors.erase(coco_exec_it);
        delete exec;
        delete slv;

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state destroyed)))").c_str(), NULL);

        e.fire_removed_solver(*coco_exec);
    }

    void send_message([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Sending message..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue topic;
        if (!UDFNextArgument(udfc, STRING_BIT, &topic))
            return;

        UDFValue message;
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        auto msg = json::load(message.lexemeValue->contents);
        e.publish(e.db.get_root() + '/' + topic.lexemeValue->contents, msg);
    }

    COCO_EXPORT coco_core::coco_core(coco_db &db) : db(db), coco_timer(1000, std::bind(&coco_core::tick, this)), env(CreateEnvironment())
    {
        AddUDF(env, "new_solver_script", "l", 3, 3, "lys", new_solver_script, "new_solver_script", NULL);
        AddUDF(env, "new_solver_files", "l", 3, 3, "lys", new_solver_files, "new_solver_files", NULL);
        AddUDF(env, "start_execution", "v", 2, 2, "ll", start_execution, "start_execution", NULL);
        AddUDF(env, "pause_execution", "v", 2, 2, "ll", pause_execution, "pause_execution", NULL);
        AddUDF(env, "delay_task", "v", 2, 3, "llm", delay_task, "delay_task", NULL);
        AddUDF(env, "extend_task", "v", 2, 3, "llm", extend_task, "extend_task", NULL);
        AddUDF(env, "failure", "v", 2, 2, "lm", failure, "failure", NULL);
        AddUDF(env, "adapt_script", "v", 2, 2, "ls", adapt_script, "adapt_script", NULL);
        AddUDF(env, "adapt_files", "v", 2, 2, "lm", adapt_files, "adapt_files", NULL);
        AddUDF(env, "delete_solver", "v", 2, 2, "ll", delete_solver, "delete_solver", NULL);
        AddUDF(env, "send_message", "v", 3, 3, "lss", send_message, "send_message", NULL);
    }
    COCO_EXPORT coco_core::~coco_core()
    {
        DestroyEnvironment(env);
    }

    COCO_EXPORT void coco_core::load_rules(const std::vector<std::string> &files)
    {
        LOG("Loading policy rules..");
        for (const auto &f : files)
            Load(env, f.c_str());

        Reset(env);

        AssertString(env, ("(configuration (coco_ptr " + std::to_string(reinterpret_cast<uintptr_t>(this)) + "))").c_str());

        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }

    COCO_EXPORT void coco_core::connect()
    {
        for (auto &mdlw : middlewares)
            mdlw->connect();
        coco_timer.start();

        db.init();
    }

    COCO_EXPORT void coco_core::init()
    {
        LOG("Initializing deduCtiOn and abduCtiOn (COCO) reasoner..");

        for (const auto &st : db.get_all_sensor_types())
            st.get().fact = AssertString(env, ("(sensor_type (id " + st.get().id + ") (name \"" + st.get().name + "\") (description \"" + st.get().description + "\"))").c_str());

        for (const auto &s : db.get_all_sensors())
        {
            std::string f_str = "(sensor (id " + s.get().id + ") (sensor_type " + s.get().type.id + ") (name \"" + s.get().name + "\")";
            if (s.get().loc)
                f_str += " (location " + std::to_string(s.get().loc->x) + " " + std::to_string(s.get().loc->y) + ")";
            f_str += ')';

            s.get().fact = AssertString(env, f_str.c_str());
        }

        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif

        for (const auto &s : db.get_all_sensors())
            for (auto &mw : middlewares)
                mw->subscribe(db.get_root() + SENSOR_TOPIC + '/' + s.get().id, 1);
    }

    COCO_EXPORT void coco_core::disconnect()
    {
        for (auto &mdlw : middlewares)
            mdlw->disconnect();
        coco_timer.stop();
    }

    COCO_EXPORT void coco_core::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        LOG("Creating new sensor type..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we store the sensor type in the database..
        auto id = db.create_sensor_type(name, description, parameter_types);
        // we create a new fact for the new sensor type..
        db.get_sensor_type(id).fact = AssertString(env, ("(sensor_type (id " + id + ") (name \"" + name + "\") (description \"" + description + "\"))").c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_new_sensor_type(db.get_sensor_type(id));
    }
    COCO_EXPORT void coco_core::set_sensor_type_name(const sensor_type &type, const std::string &name)
    {
        LOG("Setting sensor type name..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we update the sensor type in the database..
        db.set_sensor_type_name(type.id, name);
        // we update the sensor type fact..
        Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + type.id + ") (modify ?st (name \"" + name + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_sensor_type(type);
    }
    COCO_EXPORT void coco_core::set_sensor_type_description(const sensor_type &type, const std::string &description)
    {
        LOG("Setting sensor type description..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we update the sensor type in the database..
        db.set_sensor_type_description(type.id, description);
        // we update the sensor type fact
        Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + type.id + ") (modify ?st (description \"" + description + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_sensor_type(type);
    }
    COCO_EXPORT void coco_core::delete_sensor_type(const sensor_type &type)
    {
        LOG("Deleting sensor type..");
        const std::lock_guard<std::mutex> lock(mtx);
        fire_removed_sensor_type(type);
        auto f = db.get_sensor_type(type.id).fact;
        // we delete the sensor type from the database..
        db.delete_sensor_type(type.id);
        // we retract the sensor type fact..
        Retract(f);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }

    COCO_EXPORT void coco_core::create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l)
    {
        LOG("Creating new sensor..");
        const std::lock_guard<std::mutex> lock(mtx);
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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_new_sensor(db.get_sensor(id));
    }
    COCO_EXPORT void coco_core::set_sensor_name(const sensor &s, const std::string &name)
    {
        LOG("Setting sensor name..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we update the sensor in the database..
        db.set_sensor_name(s.id, name);
        // we update the sensor fact..
        Eval(env, ("(do-for-fact ((?s sensor)) (= ?s:id " + s.id + ") (modify ?s (name \"" + name + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_sensor(s);
    }
    COCO_EXPORT void coco_core::set_sensor_location(const sensor &s, std::unique_ptr<location> l)
    {
        LOG("Setting sensor location..");
        const std::lock_guard<std::mutex> lock(mtx);
        double s_x = l->x, s_y = l->y;
        // we update the sensor in the database..
        db.set_sensor_location(s.id, std::move(l));
        // we update the sensor fact..
        Eval(env, ("(do-for-fact ((?s sensor_type)) (= ?s:id " + s.id + ") (modify ?s (location " + std::to_string(s_x) + " " + std::to_string(s_y) + ")))").c_str(), NULL);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_sensor(s);
    }
    COCO_EXPORT void coco_core::delete_sensor(const sensor &s)
    {
        LOG("Deleting sensor..");
        const std::lock_guard<std::mutex> lock(mtx);
        fire_removed_sensor(s);
        auto f = db.get_sensor(s.id).fact;
        // we delete the sensor from the database..
        db.delete_sensor(s.id);
        // we retract the sensor fact..
        Retract(f);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }

    COCO_EXPORT void coco_core::publish_sensor_value(const sensor &s, const json::json &value) { publish(db.get_root() + SENSOR_TOPIC + '/' + s.id, value, 1, true); }

    COCO_EXPORT void coco_core::set_sensor_value(const sensor &s, const json::json &value)
    {
        LOG("Setting sensor value..");
        const std::lock_guard<std::mutex> lock(mtx);
        json::object &j_val = value;

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::string fact_str = "(sensor_data (sensor_id " + s.id + ") (local_time " + std::to_string(time) + ") (data";
        for (const auto &[id, val] : j_val)
        {
            json::string_val &j_v = val;
            fact_str += ' ';
            fact_str += j_v;
        }
        fact_str += "))";

        Fact *sv_f = AssertString(env, fact_str.c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif

        db.set_sensor_value(s.id, time, value);

        // we retract the sensor value fact..
        Retract(sv_f);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }

    void coco_core::tick()
    {
        for (auto &exec : executors)
            exec->tick();
    }

    void coco_core::publish(const std::string &topic, const json::json &msg, int qos, bool retained)
    {
        for (auto &mdlw : middlewares)
            mdlw->publish(topic, msg, qos, retained);
    }
    void coco_core::message_arrived(const std::string &topic, const json::json &msg)
    {
        if (topic.rfind(db.get_root() + SENSOR_TOPIC + '/', 0) == 0)
        { // we have a new sensor value..
            std::string sensor_id = topic;
            sensor_id.erase(0, (db.get_root() + SENSOR_TOPIC + '/').length());
            set_sensor_value(db.get_sensor(sensor_id), msg);
        }
        else // we notify the listeners that a message has arrived..
            fire_message_arrived(topic, msg);
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
    void coco_core::fire_removed_sensor_type(const sensor_type &st)
    {
        for (const auto &l : listeners)
            l->removed_sensor_type(st);
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
    void coco_core::fire_removed_sensor(const sensor &s)
    {
        for (const auto &l : listeners)
            l->removed_sensor(s);
    }

    void coco_core::fire_new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, json::json &value)
    {
        for (const auto &l : listeners)
            l->new_sensor_value(s, time, value);
    }

    void coco_core::fire_new_solver(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->new_solver(exec);
    }
    void coco_core::fire_removed_solver(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->removed_solver(exec);
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

    void coco_core::fire_flaw_created(const coco_executor &exec, const ratio::solver::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_created(exec, f);
    }
    void coco_core::fire_flaw_state_changed(const coco_executor &exec, const ratio::solver::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_state_changed(exec, f);
    }
    void coco_core::fire_flaw_cost_changed(const coco_executor &exec, const ratio::solver::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_cost_changed(exec, f);
    }
    void coco_core::fire_flaw_position_changed(const coco_executor &exec, const ratio::solver::flaw &f)
    {
        for (const auto &l : listeners)
            l->flaw_position_changed(exec, f);
    }
    void coco_core::fire_current_flaw(const coco_executor &exec, const ratio::solver::flaw &f)
    {
        for (const auto &l : listeners)
            l->current_flaw(exec, f);
    }

    void coco_core::fire_resolver_created(const coco_executor &exec, const ratio::solver::resolver &r)
    {
        for (const auto &l : listeners)
            l->resolver_created(exec, r);
    }
    void coco_core::fire_resolver_state_changed(const coco_executor &exec, const ratio::solver::resolver &r)
    {
        for (const auto &l : listeners)
            l->resolver_state_changed(exec, r);
    }
    void coco_core::fire_current_resolver(const coco_executor &exec, const ratio::solver::resolver &r)
    {
        for (const auto &l : listeners)
            l->current_resolver(exec, r);
    }

    void coco_core::fire_causal_link_added(const coco_executor &exec, const ratio::solver::flaw &f, const ratio::solver::resolver &r)
    {
        for (const auto &l : listeners)
            l->causal_link_added(exec, f, r);
    }

    void coco_core::fire_start_execution(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->start_execution(exec);
    }
    void coco_core::fire_pause_execution(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->pause_execution(exec);
    }

    void coco_core::fire_message_arrived(const std::string &topic, const json::json &msg)
    {
        for (const auto &l : listeners)
            l->message_arrived(topic, msg);
    }

    void coco_core::fire_tick(const coco_executor &exec, const semitone::rational &time)
    {
        for (const auto &l : listeners)
            l->tick(exec, time);
    }

    void coco_core::fire_start(const coco_executor &exec, const std::unordered_set<ratio::core::atom *> &atoms)
    {
        for (const auto &l : listeners)
            l->start(exec, atoms);
    }
    void coco_core::fire_end(const coco_executor &exec, const std::unordered_set<ratio::core::atom *> &atoms)
    {
        for (const auto &l : listeners)
            l->end(exec, atoms);
    }
} // namespace coco