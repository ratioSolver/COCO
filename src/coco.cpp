#include "coco.h"
#include "logging.h"
#include "coco_middleware.h"
#include "coco_executor.h"
#include "coco_db.h"

namespace coco
{
    void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        UDFValue coco_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &coco_ptr))
            return;
        auto &e = *reinterpret_cast<coco *>(coco_ptr.integerValue->contents);

        UDFValue solver_type;
        if (!UDFNextArgument(udfc, LEXEME_BITS, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv);
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
        auto &e = *reinterpret_cast<coco *>(coco_ptr.integerValue->contents);

        UDFValue solver_type;
        if (!UDFFirstArgument(udfc, LEXEME_BITS, &solver_type))
            return;

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv);
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
        auto &e = *reinterpret_cast<coco *>(coco_ptr.integerValue->contents);

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
        auto &e = *reinterpret_cast<coco *>(coco_ptr.integerValue->contents);

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
        auto &e = *reinterpret_cast<coco *>(coco_ptr.integerValue->contents);

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

    coco::coco(coco_db &db) : db(db), coco_timer(1000, std::bind(&coco::tick, this)), env(CreateEnvironment())
    {
        db.init();

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

        LOG("Loading policy rules..");
        Load(env, "rules/rules.clp");
        Reset(env);

        for (const auto &st : db.get_all_sensor_types())
            st.get().fact = AssertString(env, ("(sensor_type (id " + st.get().id + ") (name " + st.get().name + ") (description " + st.get().description + "))").c_str());

        for (const auto &s : db.get_all_sensors())
        {
            std::string f_str = "(sensor (id " + s.get().id + ") (sensor_type " + s.get().type.id + ") (name " + s.get().name + ")";
            if (s.get().loc)
                f_str += " (location " + std::to_string(s.get().loc->x) + " " + std::to_string(s.get().loc->y) + ")";
            f_str += ')';

            s.get().fact = AssertString(env, f_str.c_str());

            for (auto &mw : middlewares)
                mw->subscribe(db.get_root() + SENSOR_TOPIC + '/' + s.get().id, 1);
        }

        AssertString(env, ("(configuration (coco_ptr " + std::to_string(reinterpret_cast<uintptr_t>(this)) + "))").c_str());

        Run(env, -1);
#ifdef VERBOSE_LOG
        Eval(env, "(facts)", NULL);
#endif
    }
    coco::~coco()
    {
        DestroyEnvironment(env);
    }

    void coco::connect()
    {
        for (auto &mdlw : middlewares)
            mdlw->connect();
        coco_timer.start();
    }

    void coco::disconnect()
    {
        for (auto &mdlw : middlewares)
            mdlw->disconnect();
        coco_timer.stop();
    }

    void coco::tick()
    {
        for (auto &exec : executors)
            exec->tick();
    }

    void coco::publish(const std::string &topic, json::json &msg, int qos, bool retained)
    {
        for (auto &mdlw : middlewares)
            mdlw->publish(topic, msg, qos, retained);
    }
    void coco::message_arrived(json::json &msg)
    {
        json::string_val &j_msg_type = msg["type"];
        std::string msg_type = j_msg_type;

        if (msg_type == "new_sensor_type")
        { // we have a new sensor type..
            LOG("Creating new sensor type..");
            json::string_val &j_st_name = msg["name"];
            std::string st_name = j_st_name;
            json::string_val &j_st_description = msg["description"];
            std::string st_description = j_st_description;

            // we store the new sensor type on the database..
            auto id = db.create_sensor_type(st_name, st_description);
            // we create a new fact for the new sensor type..
            db.get_sensor(id).fact = AssertString(env, ("(sensor_type (id " + id + ") (name " + st_name + ") (description " + st_description + "))").c_str());
            Run(env, -1);
#ifdef VERBOSE_LOG
            Eval(env, "(facts)", NULL);
#endif
        }
        else if (msg_type == "update_sensor_type")
        {
            LOG("Updating existing sensor type..");
            json::object &j_st = msg;
            json::string_val &j_st_id = msg["id"];
            std::string st_id = j_st_id;
            if (j_st.has("name"))
            {
                json::string_val &j_st_name = msg["name"];
                std::string st_name = j_st_name;
                // we update the sensor type on the database..
                db.set_sensor_type_name(st_id, st_name);
                // we update the sensor type fact..
                Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + st_id + ") (modify ?st (name " + st_name + ")))").c_str(), NULL);
                Run(env, -1);
#ifdef VERBOSE_LOG
                Eval(env, "(facts)", NULL);
#endif
            }
            if (j_st.has("description"))
            {
                json::string_val &j_st_description = msg["description"];
                std::string st_description = j_st_description;

                // we update the sensor type on the database..
                db.set_sensor_type_description(st_id, st_description);
                // we update the sensor type fact..
                Eval(env, ("(do-for-fact ((?st sensor_type)) (= ?st:id " + st_id + ") (modify ?st (description " + st_description + ")))").c_str(), NULL);
                Run(env, -1);
#ifdef VERBOSE_LOG
                Eval(env, "(facts)", NULL);
#endif
            }
            else if (msg_type == "delete_sensor_type")
            {
                LOG("Deleting existing sensor type..");
                json::string_val &j_st_id = msg["id"];
                std::string st_id = j_st_id;
                auto f = db.get_sensor_type(st_id).fact;

                // we delete the sensor type from the database..
                db.delete_sensor_type(st_id);
                // we retract the sensor type fact..
                Retract(f);
                Run(env, -1);
#ifdef VERBOSE_LOG
                Eval(env, "(facts)", NULL);
#endif
            }
        }
    }
} // namespace coco
