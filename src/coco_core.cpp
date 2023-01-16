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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue solver_type;
        if (!UDFNextArgument(udfc, LEXEME_BITS, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv, &e.mtx);
        auto coco_exec = std::make_unique<coco_executor>(e, *exec, solver_type.lexemeValue->contents);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec.get());

        AssertString(env, std::string("(solver (solver_ptr " + std::to_string(exec_ptr) + ") (solver_type " + solver_type.lexemeValue->contents + ") (state reasoning))").c_str());

        e.executors.push_back(std::move(coco_exec));

        json::json msg;
        msg["type"] = "solvers";
        json::array solvers;
        solvers.reserve(e.executors.size());
        for (const auto &xct : e.executors)
            solvers.push_back(reinterpret_cast<uintptr_t>(xct.get()));
        msg["solvers"] = std::move(solvers);

        e.publish(e.db.get_root() + SOLVERS_TOPIC, msg, 2, true);

        // we adapt to a riddle script..
        exec->adapt(riddle.lexemeValue->contents);

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void new_solver_files(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue solver_type;
        if (!UDFFirstArgument(udfc, LEXEME_BITS, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv, &e.mtx);
        auto coco_exec = std::make_unique<coco_executor>(e, *exec, solver_type.lexemeValue->contents);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec.get());

        AssertString(env, std::string("(solver (solver_ptr " + std::to_string(exec_ptr) + ") (solver_type " + solver_type.lexemeValue->contents + ") (state reasoning))").c_str());

        e.executors.push_back(std::move(coco_exec));

        json::json msg;
        msg["type"] = "solvers";
        json::array solvers;
        solvers.reserve(e.executors.size());
        for (const auto &xct : e.executors)
            solvers.push_back(reinterpret_cast<uintptr_t>(xct.get()));
        msg["solvers"] = std::move(solvers);

        e.publish(e.db.get_root() + SOLVERS_TOPIC, msg, 2, true);

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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->start_execution();

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state executing)))").c_str(), NULL);

        json::json msg;
        msg["type"] = "start_execution";
        msg["id"] = reinterpret_cast<uintptr_t>(coco_exec);

        e.publish(e.db.get_root() + SOLVERS_TOPIC, msg, 2, true);
    }

    void pause_execution(Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Pausing plan execution..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->pause_execution();

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state paused)))").c_str(), NULL);

        json::json msg;
        msg["type"] = "pause_execution";
        msg["id"] = reinterpret_cast<uintptr_t>(coco_exec);

        e.publish(e.db.get_root() + SOLVERS_TOPIC, msg, 2, true);
    }

    void delay_task([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Delaying task..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, NUMBER_BITS, &task_id))
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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, NUMBER_BITS, &task_id))
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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco_core *>(coco_ptr.integerValue->contents);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
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

        json::json msg;
        msg["type"] = "deleted_reasoner";
        msg["id"] = reinterpret_cast<uintptr_t>(coco_exec);

        e.publish(e.db.get_root() + SOLVERS_TOPIC, msg, 2, true);
    }

    void send_message([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Sending message..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
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
        for (const auto &st : db.get_all_sensor_types())
            st.get().fact = AssertString(env, ("(sensor_type (id " + st.get().id + ") (name \"" + st.get().name + "\") (description \"" + st.get().description + "\"))").c_str());

        for (const auto &s : db.get_all_sensors())
        {
            std::string f_str = "(sensor (id " + s.get().id + ") (sensor_type " + s.get().type.id + ") (name \"" + s.get().name + "\")";
            if (s.get().loc)
                f_str += " (location " + std::to_string(s.get().loc->x) + " " + std::to_string(s.get().loc->y) + ")";
            f_str += ')';

            s.get().fact = AssertString(env, f_str.c_str());

            for (auto &mw : middlewares)
                mw->subscribe(db.get_root() + SENSOR_TOPIC + '/' + s.get().id, 1);
        }

        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }

    COCO_EXPORT void coco_core::disconnect()
    {
        for (auto &mdlw : middlewares)
            mdlw->disconnect();
        coco_timer.stop();
    }

    COCO_EXPORT void coco_core::create_sensor_type(const std::string &name, const std::string &description)
    {
        LOG("Creating new sensor type..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we store the sensor type in the database..
        auto id = db.create_sensor_type(name, description);
        // we create a new fact for the new sensor type..
        db.get_sensor_type(id).fact = AssertString(env, ("(sensor_type (id " + id + ") (name \"" + name + "\") (description \"" + description + "\"))").c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        // we publish the new sensor type to the network..
        json::json msg;
        msg["type"] = "sensor_type_created";
        msg["id"] = id;
        msg["name"] = name;
        msg["description"] = description;
        publish(db.get_root() + SENSORS_TOPIC, msg);
    }
    COCO_EXPORT void coco_core::set_sensor_type_name(const sensor_type &type, const std::string &name)
    {
        LOG("Setting sensor type name..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we update the sensor type in the database..
        db.set_sensor_type_name(type.id, name);
        // we update the sensor type fact..
        Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + type.id + ") (modify ?st (name \"" + name + "\")))").c_str(), NULL);
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        // we publish the new sensor type to the network..
        json::json msg;
        msg["type"] = "sensor_type_updated";
        msg["id"] = type.id;
        msg["name"] = name;
        publish(db.get_root() + SENSORS_TOPIC, msg);
    }
    COCO_EXPORT void coco_core::set_sensor_type_description(const sensor_type &type, const std::string &description)
    {
        LOG("Setting sensor type description..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we update the sensor type in the database..
        db.set_sensor_type_description(type.id, description);
        // we update the sensor type fact
        Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + type.id + ") (modify ?st (description \"" + description + "\")))").c_str(), NULL);
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        // we publish the new sensor type to the network..
        json::json msg;
        msg["type"] = "sensor_type_updated";
        msg["id"] = type.id;
        msg["description"] = description;
        publish(db.get_root() + SENSORS_TOPIC, msg);
    }
    COCO_EXPORT void coco_core::delete_sensor_type(const sensor_type &type)
    {
        LOG("Deleting sensor type..");
        const std::lock_guard<std::mutex> lock(mtx);
        auto f = db.get_sensor_type(type.id).fact;
        // we delete the sensor type from the database..
        db.delete_sensor_type(type.id);
        // we retract the sensor type fact..
        Retract(f);
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
        db.get_sensor(id).fact = AssertString(env, ("(sensor (id " + id + ") (sensor_type " + type.id + ") (name \"" + name + "\")").c_str());
        if (l)
            AssertString(env, (" (location " + std::to_string(l->x) + " " + std::to_string(l->y) + ")").c_str());
        AssertString(env, ")");
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        // we publish the new sensor to the network..
        json::json msg;
        msg["type"] = "sensor_created";
        msg["id"] = id;
        msg["name"] = name;
        msg["sensor_type"] = type.id;
        if (l)
        {
            msg["location"]["x"] = l->x;
            msg["location"]["y"] = l->y;
        }
        publish(db.get_root() + SENSORS_TOPIC, msg);
    }
    COCO_EXPORT void coco_core::set_sensor_name(const sensor &s, const std::string &name)
    {
        LOG("Setting sensor name..");
        const std::lock_guard<std::mutex> lock(mtx);
        // we update the sensor in the database..
        db.set_sensor_name(s.id, name);
        // we update the sensor fact..
        Eval(env, ("(do-for-fact ((?s sensor)) (= ?s:id " + s.id + ") (modify ?s (name \"" + name + "\")))").c_str(), NULL);
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        // we publish the new sensor to the network..
        json::json msg;
        msg["type"] = "sensor_updated";
        msg["id"] = s.id;
        msg["name"] = name;
        publish(db.get_root() + SENSORS_TOPIC, msg);
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
        // we publish the new sensor to the network..
        json::json msg;
        msg["type"] = "sensor_updated";
        msg["id"] = s.id;
        json::json loc;
        loc["x"] = s_x;
        loc["y"] = s_y;
        msg["location"] = std::move(loc);
        publish(db.get_root() + SENSORS_TOPIC, msg);
    }
    COCO_EXPORT void coco_core::delete_sensor(const sensor &s)
    {
        LOG("Deleting sensor..");
        const std::lock_guard<std::mutex> lock(mtx);
        auto f = db.get_sensor(s.id).fact;
        // we delete the sensor from the database..
        db.delete_sensor(s.id);
        // we retract the sensor fact..
        Retract(f);
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        // we publish the new sensor to the network..
        json::json msg;
        msg["type"] = "sensor_deleted";
        msg["id"] = s.id;
        publish(db.get_root() + SENSORS_TOPIC, msg);
    }

    COCO_EXPORT void coco_core::set_sensor_value(const sensor &s, json::json &value, bool republish)
    {
        if (republish)
            publish(db.get_root() + SENSOR_TOPIC + '/' + s.id, value, 1, true);

        LOG("Setting sensor value..");
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
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif

        db.set_sensor_value(s.id, time, value);

        Retract(sv_f);
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

    void coco_core::publish(const std::string &topic, json::json &msg, int qos, bool retained)
    {
        for (auto &mdlw : middlewares)
            mdlw->publish(topic, msg, qos, retained);
    }
    void coco_core::message_arrived(const std::string &topic, json::json &msg)
    {
        const std::lock_guard<std::mutex> lock(mtx);

        if (topic.rfind(db.get_root() + SENSOR_TOPIC + '/', 0) == 0)
        { // we have a new sensor value..
            std::string sensor_id = topic;
            sensor_id.erase(0, (db.get_root() + SENSOR_TOPIC + '/').length());
            set_sensor_value(db.get_sensor(sensor_id), msg, false);
        }

        // we notify the listeners that a message has arrived..
        fire_message_arrived(topic, msg);
    }

    void coco_core::fire_new_solver(const coco_executor &exec)
    {
        for (const auto &l : listeners)
            l->new_solver(exec);
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

    void coco_core::fire_message_arrived(const std::string &topic, json::json &msg)
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
