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
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &solver_type))
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

        e.executors.push_back(coco_exec);

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
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &solver_type))
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

        e.executors.push_back(coco_exec);

        // we adapt to some riddle files..
        exec->adapt(fs);

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
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
        {
            auto &file = riddle.multifieldValue->contents[i];
            if (file.header->type != STRING_TYPE)
                return;
            fs.push_back(file.lexemeValue->contents);
        }

        // we adapt to some riddle files..
        exec->adapt(fs);
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

        auto coco_exec_it = std::find_if(e.executors.cbegin(), e.executors.cend(), [coco_exec](auto &slv_ptr)
                                         { return &*slv_ptr == coco_exec; });
        e.executors.erase(coco_exec_it);
        delete exec;
        delete slv;

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (retract ?slv))").c_str(), NULL);

        e.fire_removed_solver(*coco_exec);
    }

    void publish_message([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
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
        AddUDF(env, "new_solver_files", "l", 3, 3, "lym", new_solver_files, "new_solver_files", NULL);
        AddUDF(env, "start_execution", "v", 1, 1, "l", start_execution, "start_execution", NULL);
        AddUDF(env, "pause_execution", "v", 1, 1, "l", pause_execution, "pause_execution", NULL);
        AddUDF(env, "delay_task", "v", 2, 3, "llm", delay_task, "delay_task", NULL);
        AddUDF(env, "extend_task", "v", 2, 3, "llm", extend_task, "extend_task", NULL);
        AddUDF(env, "failure", "v", 2, 2, "lm", failure, "failure", NULL);
        AddUDF(env, "adapt_script", "v", 2, 2, "ls", adapt_script, "adapt_script", NULL);
        AddUDF(env, "adapt_files", "v", 2, 2, "lm", adapt_files, "adapt_files", NULL);
        AddUDF(env, "delete_solver", "v", 1, 1, "l", delete_solver, "delete_solver", NULL);
        AddUDF(env, "publish_message", "v", 3, 3, "lss", publish_message, "publish_message", NULL);
    }
    COCO_EXPORT coco_core::~coco_core()
    {
        DestroyEnvironment(env);
    }

    COCO_EXPORT void coco_core::load_rules(const std::vector<std::string> &files)
    {
        LOG_DEBUG("Loading policy rules..");
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
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        for (auto &mdlw : middlewares)
            mdlw->connect();
        coco_timer.start();

        db.init();
    }

    COCO_EXPORT void coco_core::init()
    {
        LOG("Initializing deduCtiOn and abduCtiOn (COCO) reasoner..");

        for (const auto &st : db.get_sensor_types())
            st.get().fact = AssertString(env, ("(sensor_type (id " + st.get().id + ") (name \"" + st.get().name + "\") (description \"" + st.get().description + "\"))").c_str());

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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif

        for (const auto &s : db.get_sensors())
        {
            LOG_DEBUG("Managing sensor (" << s.get().id << ") '" << s.get().name << "' of type '" << s.get().type.name << "' subscriptions..");
            for (auto &mw : middlewares)
                mw->subscribe(db.get_root() + SENSOR_TOPIC + '/' + s.get().id, 1);
        }
    }

    COCO_EXPORT void coco_core::disconnect()
    {
        for (auto &mdlw : middlewares)
            mdlw->disconnect();
        coco_timer.stop();
    }

    COCO_EXPORT void coco_core::create_user(const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &roots, const json::json &data)
    {
        LOG_DEBUG("Creating new user..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we store the user in the database..
        auto id = db.create_user(first_name, last_name, email, password, roots, data);
        if (db.has_user(id))
        {
            // we create a new fact for the new user..
            db.get_user(id).fact = AssertString(env, ("(user (id " + id + ") (first_name \"" + first_name + "\") (last_name \"" + last_name + "\") (last_name \"" + email + "\"))").c_str());
            // we run the rules engine to update the policy..
            Run(env, -1);
#ifdef VERBOSE_LOG
            Eval(env, "(facts)", NULL);
#endif
            fire_new_user(db.get_user(id));
        }
    }
    COCO_EXPORT void coco_core::set_user_first_name(user &u, const std::string &first_name)
    {
        LOG_DEBUG("Setting user first name..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the user in the database..
        db.set_user_first_name(u, first_name);
        // we update the user fact..
        Eval(env, ("(do-for-fact ((?u user)) ((eq ?u:id \"" + u.id + "\")) (modify ?u (first_name \"" + first_name + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_user(u);
    }
    COCO_EXPORT void coco_core::set_user_last_name(user &u, const std::string &last_name)
    {
        LOG_DEBUG("Setting user last name..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the user in the database..
        db.set_user_last_name(u, last_name);
        // we update the user fact..
        Eval(env, ("(do-for-fact ((?u user)) ((eq ?u:id \"" + u.id + "\")) (modify ?u (last_name \"" + last_name + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_user(u);
    }
    COCO_EXPORT void coco_core::set_user_email(user &u, const std::string &email)
    {
        LOG_DEBUG("Setting user email..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the user in the database..
        db.set_user_email(u, email);
        // we update the user fact..
        Eval(env, ("(do-for-fact ((?u user)) ((eq ?u:id \"" + u.id + "\")) (modify ?u (email \"" + email + "\")))").c_str(), NULL);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_user(u);
    }
    COCO_EXPORT void coco_core::set_user_password(user &u, const std::string &password)
    {
        LOG_DEBUG("Setting user password..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the user in the database..
        db.set_user_password(u, password);
        fire_updated_user(u);
    }
    COCO_EXPORT void coco_core::set_user_roots(user &u, const std::vector<std::string> &roots)
    {
        LOG_DEBUG("Setting user roots..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the user in the database..
        db.set_user_roots(u, roots);
        fire_updated_user(u);
    }
    COCO_EXPORT void coco_core::set_user_data(user &u, const json::json &data)
    {
        LOG_DEBUG("Setting user data..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        // we update the user in the database..
        db.set_user_data(u, data);
        fire_updated_user(u);
    }
    COCO_EXPORT void coco_core::delete_user(user &u)
    {
        LOG_DEBUG("Deleting user..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        fire_removed_user(u);
        if (db.has_user(u.id))
        {
            // we retract the user fact..
            Retract(u.fact);
            // we run the rules engine to update the policy..
            Run(env, -1);
#ifdef VERBOSE_LOG
            Eval(env, "(facts)", NULL);
#endif
        }
        // we delete the user from the database..
        db.delete_user(u);
    }

    COCO_EXPORT void coco_core::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        LOG_DEBUG("Creating new sensor type..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_sensor_type(type);
    }
    COCO_EXPORT void coco_core::delete_sensor_type(sensor_type &type)
    {
        LOG_DEBUG("Deleting sensor type..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        fire_removed_sensor_type(type);
        auto f = db.get_sensor_type(type.id).fact;
        // we delete the sensor type from the database..
        db.delete_sensor_type(type);
        // we retract the sensor type fact..
        Retract(f);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_new_sensor(db.get_sensor(id));

        // we subscribe to the sensor topic..
        LOG_DEBUG("Managing sensor (" << id << ") " << name << " of type " << type.id << " subscriptions..");
        for (auto &mw : middlewares)
            mw->subscribe(db.get_root() + SENSOR_TOPIC + '/' + id, 1);
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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
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
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
        fire_updated_sensor(s);
    }
    COCO_EXPORT void coco_core::delete_sensor(sensor &s)
    {
        LOG_DEBUG("Deleting sensor..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        fire_removed_sensor(s);
        auto f = db.get_sensor(s.id).fact;
        // we delete the sensor from the database..
        db.delete_sensor(s);
        // we retract the sensor fact..
        Retract(f);
        // we run the rules engine to update the policy..
        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }

    COCO_EXPORT void coco_core::publish_sensor_value(const sensor &s, const json::json &value)
    {
        LOG_DEBUG("Publishing sensor value..");
        publish(db.get_root() + SENSOR_TOPIC + '/' + s.id, value, 1, true);
    }

    COCO_EXPORT void coco_core::publish_random_value(const sensor &s)
    {
        LOG_DEBUG("Publishing random value..");
        json::json value;
        for (const auto &[name, type] : s.get_type().get_parameters())
            switch (type)
            {
            case parameter_type::Integer:
                value[name] = (long)rand() % 100;
                break;
            case parameter_type::Float:
                value[name] = (float)rand() / (float)RAND_MAX;
                break;
            case parameter_type::Boolean:
                value[name] = rand() % 2 == 0;
                break;
            case parameter_type::Symbol:
                value[name] = "test";
                break;
            case parameter_type::String:
                value[name] = "test";
                break;
            }

        publish(db.get_root() + SENSOR_TOPIC + '/' + s.id, value, 1, true);
    }

    void coco_core::set_sensor_value(sensor &s, const json::json &value)
    {
        LOG_DEBUG("Setting sensor value..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        fire_new_sensor_value(s, time, value);

        std::string fact_str = "(sensor_data (sensor_id " + s.id + ") (local_time " + std::to_string(time) + ") (data";
        for (const auto &[id, val] : value.get_object())
            if (s.get_type().has_parameter(id))
                fact_str += ' ' + val.to_string();
            else
                LOG_ERR("Sensor " << s.id << " does not have parameter " << id);
        fact_str += "))";
        LOG_DEBUG("Asserting fact: " << fact_str);

        Fact *sv_f = AssertString(env, fact_str.c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);

        db.set_sensor_value(s, time, value);

        // we retract the sensor value fact..
        Retract(sv_f);
        // we run the rules engine to update the policy..
        Run(env, -1);
    }

    void coco_core::set_sensor_state(sensor &s, const json::json &state)
    {
        LOG_DEBUG("Setting sensor state..");
        const std::lock_guard<std::recursive_mutex> lock(mtx);

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        fire_new_sensor_state(s, time, state);

        std::string fact_str = "(sensor_state (sensor_id " + s.id + ") (state" + state.to_string() + "))";
        LOG_DEBUG("Asserting fact: " << fact_str);

        Fact *ss_f = AssertString(env, fact_str.c_str());
        // we run the rules engine to update the policy..
        Run(env, -1);
        // we retract the sensor state fact..
        Retract(ss_f);
        // we run the rules engine to update the policy..
        Run(env, -1);
    }

    void coco_core::tick()
    {
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        std::vector<coco_executor *> c_executors;
        c_executors.reserve(executors.size());
        for (auto &exec : executors)
            c_executors.push_back(exec.operator->());
        for (auto &exec : c_executors)
            exec->tick();
    }

    void coco_core::publish(const std::string &topic, const json::json &msg, int qos, bool retained)
    {
        for (auto &mdlw : middlewares)
            mdlw->publish(topic, msg, qos, retained);
    }
    void coco_core::message_arrived(const std::string &topic, const json::json &msg)
    {
        const std::lock_guard<std::recursive_mutex> lock(mtx);
        if (topic.rfind(db.get_root() + SENSOR_TOPIC + '/', 0) == 0)
        { // we have a new sensor value..
            std::string sensor_id = topic;
            sensor_id.erase(0, (db.get_root() + SENSOR_TOPIC + '/').length());
            set_sensor_value(db.get_sensor(sensor_id), msg);
        }
        else if (topic.rfind(db.get_root() + SENSOR_STATE + '/', 0) == 0)
        { // we have a new sensor state..
            std::string sensor_id = topic;
            sensor_id.erase(0, (db.get_root() + SENSOR_TOPIC + '/').length());
            set_sensor_state(db.get_sensor(sensor_id), msg);
        }
        else // we notify the listeners that a message has arrived..
            fire_message_arrived(topic, msg);
    }

    void coco_core::fire_new_user(const user &u)
    {
        for (const auto &l : listeners)
            l->new_user(u);
    }
    void coco_core::fire_updated_user(const user &u)
    {
        for (const auto &l : listeners)
            l->updated_user(u);
    }
    void coco_core::fire_removed_user(const user &u)
    {
        for (const auto &l : listeners)
            l->removed_user(u);
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

    void coco_core::fire_new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &value)
    {
        for (const auto &l : listeners)
            l->new_sensor_value(s, time, value);
    }
    void coco_core::fire_new_sensor_state(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &state)
    {
        for (const auto &l : listeners)
            l->new_sensor_state(s, time, state);
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
} // namespace coco
