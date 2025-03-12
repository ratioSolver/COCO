#include "coco_transformer.hpp"
#include "coco_item.hpp"
#include "logging.hpp"

namespace coco
{
    transformer::transformer(coco &cc, std::string_view host, unsigned short port) noexcept : listener(cc), client(host, port)
    {
        LOG_TRACE(intent_deftemplate);
        Build(get_env(), intent_deftemplate);

        AddUDF(get_env(), "understand", "v", 1, 1, "s", understand, "understand", this);
        AddUDF(get_env(), "trigger_intent", "v", 3, 5, "yyymm", trigger_intent, "trigger_intent", this);
        AddUDF(get_env(), "compute_response", "v", 3, 3, "yys", compute_response, "compute_response", this);

        auto res = client.get("/version");
        if (!res || res->get_status_code() != network::ok)
            LOG_ERR("Failed to connect to the transformer server");
        else
            LOG_DEBUG("Connected to the transformer server " << static_cast<network::json_response &>(*res).get_body());
    }

    void understand(Environment *env, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Understanding..");

        auto &t = *reinterpret_cast<transformer *>(udfc->context);

        UDFValue message;
        if (!UDFFirstArgument(udfc, STRING_BIT, &message))
            return;

        auto res = t.client.post("/model/parse", {{"text", message.lexemeValue->contents}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        std::string intent = json_res.get_body()["intent"]["name"];
        double confidence = json_res.get_body()["intent"]["confidence"];
        auto &entities = json_res.get_body()["entities"].as_array();

        FunctionCallBuilder *intent_builder = CreateFunctionCallBuilder(env, 5);
        FCBAppendSymbol(intent_builder, intent.c_str());
        FCBAppendFloat(intent_builder, confidence);
        auto es = CreateMultifieldBuilder(env, entities.size());
        auto vs = CreateMultifieldBuilder(env, entities.size());
        auto cs = CreateMultifieldBuilder(env, entities.size());
        for (const auto &entity : entities)
        {
            MBAppendSymbol(es, static_cast<std::string>(entity["entity"]).c_str());
            MBAppendSymbol(vs, static_cast<std::string>(entity["value"]).c_str());
            MBAppendFloat(cs, entity["confidence"]);
        }
        FCBAppendMultifield(intent_builder, MBCreate(es));
        FCBAppendMultifield(intent_builder, MBCreate(vs));
        FCBAppendMultifield(intent_builder, MBCreate(cs));
        FCBCall(intent_builder, "intent", nullptr);
        FCBDispose(intent_builder);
    }
    void trigger_intent(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_TRACE("Triggering intent..");

        auto &t = *reinterpret_cast<transformer *>(udfc->context);

        UDFValue ctx;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &ctx))
            return;

        UDFValue item;
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &item))
            return;

        UDFValue intent;
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &intent))
            return;

        auto &itm = t.cc.get_item(item.lexemeValue->contents);
        LOG_DEBUG("[" << ctx.lexemeValue->contents << "] intent " << intent.lexemeValue->contents << " triggered on " << itm.get_id());

        json::json body{{"name", intent.lexemeValue->contents}};

        if (UDFHasNextArgument(udfc))
        {
            UDFValue entities, values;
            if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &entities))
                return;
            if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &values))
                return;

            if (entities.multifieldValue->length != values.multifieldValue->length)
                return;

            json::json entities_json;
            for (size_t i = 0; i < entities.multifieldValue->length; ++i)
            {
                auto &entity = entities.multifieldValue->contents[i];
                if (entity.header->type != SYMBOL_TYPE)
                    return;
                auto &value = values.multifieldValue->contents[i];
                switch (value.header->type)
                {
                case INTEGER_TYPE:
                    entities_json[entity.lexemeValue->contents] = static_cast<int64_t>(value.integerValue->contents);
                    break;
                case FLOAT_TYPE:
                    entities_json[entity.lexemeValue->contents] = value.floatValue->contents;
                    break;
                case STRING_TYPE:
                case SYMBOL_TYPE:
                    entities_json[entity.lexemeValue->contents] = value.lexemeValue->contents;
                    break;
                default:
                    return;
                }
            }
            LOG_DEBUG("Entities: " << entities_json.dump());
            body["entities"] = std::move(entities_json);
        }

        std::string url = "/conversations/";
        url += ctx.lexemeValue->contents;
        url += "/trigger_intent";
        auto res = t.client.post(std::move(url), std::move(body));
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to trigger intent..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        auto &messages = json_res.get_body()["messages"].as_array();
        for (auto &message : messages)
        {
            json::json data;
            for (auto &[key, value] : message.as_object())
                if (key == "custom")
                    for (auto &[k, v] : value.as_object())
                        data[k] = v;
                else if (key != "recipient_id")
                    data[key] = value;
            t.cc.set_value(itm, std::move(data));
        }
    }
    void compute_response(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_TRACE("Computing response..");

        auto &t = *reinterpret_cast<transformer *>(udfc->context);

        UDFValue ctx;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &ctx))
            return;

        UDFValue item;
        if (!UDFNextArgument(udfc, STRING_BIT, &item))
            return;

        UDFValue message;
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        auto &itm = t.cc.get_item(item.lexemeValue->contents);
        LOG_DEBUG("[" << ctx.lexemeValue->contents << "] says: '" << message.lexemeValue->contents << "' on " << itm.get_id());

        auto res = t.client.post("/webhooks/rest/webhook", {{"sender", ctx.lexemeValue->contents}, {"message", message.lexemeValue->contents}}, {{"Content-Type", "application/json"}, {"Connection", "keep-alive"}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to compute response..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        for (auto &response : json_res.get_body().as_array())
        {
            json::json data;
            for (auto &[key, value] : response.as_object())
                if (key == "custom")
                    for (auto &[k, v] : value.as_object())
                        data[k] = v;
                else if (key != "recipient_id")
                    data[key] = value;
            t.cc.set_value(itm, std::move(data));
        }
    }
} // namespace coco
