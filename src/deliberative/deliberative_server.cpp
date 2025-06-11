#include "deliberative_server.hpp"
#include "coco_executor.hpp"

namespace coco
{
    deliberative_server::deliberative_server(coco_server &srv, coco_deliberative &cd) noexcept : server_module(srv), deliberative_listener(cd)
    {
        srv.add_route(network::Get, "^/deliberative_rules$", std::bind(&deliberative_server::get_deliberative_rules, this, network::placeholders::request));
        srv.add_route(network::Post, "^/deliberative_rules$", std::bind(&deliberative_server::create_deliberative_rule, this, network::placeholders::request));
    }

    std::unique_ptr<network::response> deliberative_server::get_deliberative_rules(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &dr : cd.get_deliberative_rules())
        {
            auto j_drs = dr->to_json();
            j_drs["name"] = dr->get_name();
            is.push_back(std::move(j_drs));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }
    std::unique_ptr<network::response> deliberative_server::create_deliberative_rule(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string || !body.contains("content") || body["content"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string content = body["content"];
        try
        {
            cd.create_deliberative_rule(name, content);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }

    void deliberative_server::executor_created(coco_executor &exec)
    {
        json::json j_exec = exec.to_json();
        j_exec["msg_type"] = "new_executor";
        j_exec["executor"] = static_cast<uint64_t>(exec.get_id());
        srv.broadcast(std::move(j_exec));
    }
    void deliberative_server::executor_deleted(coco_executor &exec)
    {
        json::json j_exec = {{"msg_type", "executor_deleted"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        srv.broadcast(std::move(j_exec));
    }

    void deliberative_server::state_changed(coco_executor &exec)
    {
        json::json j_exec = exec.to_json();
        j_exec["msg_type"] = "state_changed";
        j_exec["executor"] = static_cast<uint64_t>(exec.get_id());
        srv.broadcast(std::move(j_exec));
    }

    void deliberative_server::flaw_created(coco_executor &exec, const ratio::flaw &f)
    {
        json::json j_msg = f.to_json();
        j_msg["msg_type"] = "flaw_created";
        j_msg["executor"] = static_cast<uint64_t>(exec.get_id());
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::flaw_state_changed(coco_executor &exec, const ratio::flaw &f)
    {
        json::json j_msg = {{"msg_type", "flaw_state_changed"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"id", f.get_id()}, {"state", ratio::to_string(f.get_state())}};
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::flaw_cost_changed(coco_executor &exec, const ratio::flaw &f)
    {
        auto cost = f.get_estimated_cost();
        json::json j_msg = {{"msg_type", "flaw_cost_changed"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"id", f.get_id()}, {"cost", {{"num", cost.numerator()}, {"den", cost.denominator()}}}};
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::flaw_position_changed(coco_executor &exec, const ratio::flaw &f)
    {
        json::json j_msg = {{"msg_type", "flaw_position_changed"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"id", f.get_id()}, {"position", f.get_position()}};
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::current_flaw(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::flaw>> f)
    {
        json::json j_msg = {{"msg_type", "current_flaw"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        if (f)
            j_msg["id"] = static_cast<uint64_t>(f.value()->get_id());
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::resolver_created(coco_executor &exec, const ratio::resolver &r)
    {
        json::json j_r = r.to_json();
        j_r["msg_type"] = "resolver_created";
        j_r["executor"] = static_cast<uint64_t>(exec.get_id());
        srv.broadcast(std::move(j_r));
    }
    void deliberative_server::resolver_state_changed(coco_executor &exec, const ratio::resolver &r)
    {
        json::json j_r = {{"msg_type", "resolver_state_changed"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"id", r.get_id()}, {"state", ratio::to_string(r.get_state())}};
        srv.broadcast(std::move(j_r));
    }
    void deliberative_server::current_resolver(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::resolver>> r)
    {
        json::json j_r = {{"msg_type", "current_resolver"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        if (r)
            j_r["id"] = static_cast<uint64_t>(r.value()->get_id());
        srv.broadcast(std::move(j_r));
    }
    void deliberative_server::causal_link_added(coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r)
    {
        json::json j_msg = {{"msg_type", "causal_link_added"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"flaw", f.get_id()}, {"resolver", r.get_id()}};
        srv.broadcast(std::move(j_msg));
    }

    void deliberative_server::executor_state_changed(coco_executor &exec, ratio::executor::executor_state state)
    {
        json::json j_msg = {{"msg_type", "executor_state_changed"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"state", static_cast<int>(state)}};
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::tick(coco_executor &exec, const utils::rational &time)
    {
        json::json j_msg = {{"msg_type", "tick"}, {"executor", static_cast<uint64_t>(exec.get_id())}, {"time", {{"num", time.numerator()}, {"den", time.denominator()}}}};
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::starting(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        json::json j_msg = {{"msg_type", "starting"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        json::json j_atms(json::json_type::array);
        for (auto &atm : atms)
            j_atms.push_back(atm->to_json());
        j_msg["atms"] = std::move(j_atms);
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::start(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        json::json j_msg = {{"msg_type", "start"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        json::json j_atms(json::json_type::array);
        for (auto &atm : atms)
            j_atms.push_back(atm->to_json());
        j_msg["atms"] = std::move(j_atms);
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::ending(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        json::json j_msg = {{"msg_type", "ending"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        json::json j_atms(json::json_type::array);
        for (auto &atm : atms)
            j_atms.push_back(atm->to_json());
        j_msg["atms"] = std::move(j_atms);
        srv.broadcast(std::move(j_msg));
    }
    void deliberative_server::end(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        json::json j_msg = {{"msg_type", "end"}, {"executor", static_cast<uint64_t>(exec.get_id())}};
        json::json j_atms(json::json_type::array);
        for (auto &atm : atms)
            j_atms.push_back(atm->to_json());
        j_msg["atms"] = std::move(j_atms);
        srv.broadcast(std::move(j_msg));
    }
} // namespace coco
