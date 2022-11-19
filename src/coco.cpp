#include "coco.h"
#include "logging.h"
#include "coco_middleware.h"
#include "coco_executor.h"

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

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);

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

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);

        // we adapt to some riddle files..
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
            fs.push_back(riddle.multifieldValue->contents[i].lexemeValue->contents);
        exec->adapt(fs);

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void start_execution(Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
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
        coco_exec->start_execution();

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state executing)))").c_str(), NULL);

        json::json msg;
        msg["type"] = "start_execution";
        msg["id"] = reinterpret_cast<uintptr_t>(coco_exec);

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);
    }

    void pause_execution(Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
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
        coco_exec->pause_execution();

        Eval(env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(exec_ptr.integerValue->contents) + ") (modify ?slv (state paused)))").c_str(), NULL);

        json::json msg;
        msg["type"] = "pause_execution";
        msg["id"] = reinterpret_cast<uintptr_t>(coco_exec);

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);
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

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);
    }

    coco::coco(const std::string &root, const std::string &mongodb_uri) : root(root), conn{mongocxx::uri{mongodb_uri}}, db(conn[root]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["coco"]), sensor_data_collection(db["sensor_data"]), coco_timer(1000, std::bind(&coco::tick, this)), env(CreateEnvironment())
    {
        AddUDF(env, "new_solver_script", "lys", 3, 3, "lys", new_solver_script, "new_solver_script", NULL);
        AddUDF(env, "new_solver_files", "lym", 3, 3, "lys", new_solver_files, "new_solver_files", NULL);
        AddUDF(env, "read_script", "v", 2, 2, "ls", adapt_script, "adapt_script", NULL);
        AddUDF(env, "read_files", "v", 2, 2, "lm", adapt_files, "adapt_files", NULL);
        AddUDF(env, "delete_solver", "v", 2, 2, "ll", delete_solver, "delete_solver", NULL);

        LOG("Loading policy rules..");
        Load(env, "rules/rules.clp");
        Reset(env);

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

    void coco::publish(const std::string &topic, const json::json &msg, int qos, bool retained)
    {
        for (auto &mdlw : middlewares)
            mdlw->publish(topic, msg, qos, retained);
    }
    void coco::message_arrived(const json::json &msg)
    {
    }
} // namespace coco
