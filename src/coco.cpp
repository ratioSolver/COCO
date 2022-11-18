#include "coco.h"
#include "logging.h"
#include "coco_middleware.h"
#include "coco_executor.h"

namespace coco
{
    void new_solver(Environment *env, UDFContext *udfc, UDFValue *out)
    {
        LOG_DEBUG("Creating new solver..");

        UDFValue network_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &network_ptr))
            return;
        auto &e = *reinterpret_cast<coco *>(network_ptr.integerValue->contents);

        auto slv = new ratio::solver::solver();
        auto exec = new ratio::executor::executor(*slv);
        auto coco_exec = std::make_unique<coco_executor>(e, *exec);
        uintptr_t exec_ptr = reinterpret_cast<uintptr_t>(coco_exec.get());

        e.executors.push_back(std::move(coco_exec));

        json::json msg;
        msg["type"] = "solvers";
        json::array solvers;
        solvers.reserve(e.executors.size());
        for (const auto &exec : e.executors)
            solvers.push_back(reinterpret_cast<uintptr_t>(exec.get()));
        msg["solvers"] = std::move(solvers);

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);

        out->integerValue = CreateInteger(env, exec_ptr);
    }

    void read_script([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Reading RiDDLe snippet..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();
        auto slv = &exec->get_solver();

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        // we read a riddle script..
        slv->read(riddle.lexemeValue->contents);
        slv->solve();
    }

    void read_files([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Reading RiDDLe files..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();
        auto slv = &exec->get_solver();

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;

        // we read some riddle files..
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
            fs.push_back(riddle.multifieldValue->contents[i].lexemeValue->contents);
        slv->read(fs);
        slv->solve();
    }

    void adapt_script([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Adapting to RiDDLe snippet..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
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
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        auto exec = &coco_exec->get_executor();

        UDFValue riddle;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle))
            return;

        // we adapt to some riddle files..
        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle.multifieldValue->length; ++i)
            fs.push_back(riddle.multifieldValue->contents[i].lexemeValue->contents);
        exec->adapt(fs);
    }

    void delete_solver([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out)
    {
        LOG_DEBUG("Deleting solver..");

        UDFValue network_ptr;
        if (!UDFFirstArgument(udfc, NUMBER_BITS, &network_ptr))
            return;
        auto &e = *reinterpret_cast<coco *>(network_ptr.integerValue->contents);

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

        json::json msg;
        msg["type"] = "deleted_reasoner";
        msg["id"] = reinterpret_cast<uintptr_t>(coco_exec);

        e.publish(e.root + SOLVERS_TOPIC, msg, 2, true);
    }

    coco::coco(const std::string &root, const std::string &mqtt_uri, const std::string &mongodb_uri) : root(root), conn{mongocxx::uri{mongodb_uri}}, db(conn[root]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["coco"]), sensor_data_collection(db["sensor_data"]), coco_timer(1000, std::bind(&coco::tick, this)), env(CreateEnvironment())
    {
        AddUDF(env, "new_solver", "l", 1, 1, "l", new_solver, "new_solver", NULL);
        AddUDF(env, "read_script", "v", 2, 2, "ls", read_script, "read_script", NULL);
        AddUDF(env, "read_files", "v", 2, 2, "lm", read_files, "read_files", NULL);
        AddUDF(env, "read_script", "v", 2, 2, "ls", adapt_script, "adapt_script", NULL);
        AddUDF(env, "read_files", "v", 2, 2, "lm", adapt_files, "adapt_files", NULL);
        AddUDF(env, "delete_solver", "v", 2, 2, "ll", delete_solver, "delete_solver", NULL);

        LOG("Loading policy rules..");
        Load(env, "rules/rules.clp");
        Reset(env);

        AssertString(env, ("(configuration (network_ptr " + std::to_string(reinterpret_cast<uintptr_t>(this)) + "))").c_str());

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
