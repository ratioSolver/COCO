#include <bsoncxx/builder/basic/array.hpp>
#include "bson_parameter_converter.hpp"
#include "mongo_db.hpp"

namespace coco
{
    bsoncxx::builder::basic::document bson_parameter_converter::to_bson(const mongo_db &db, const coco_parameter &p) { return db.to_bson(p); }
    std::unique_ptr<coco_parameter> bson_parameter_converter::from_bson(const mongo_db &db, const bsoncxx::v_noabi::document::view &doc) { return db.from_bson(doc); }

    bsoncxx::builder::basic::document integer_parameter_converter::to_bson(const coco_parameter &p) const
    {
        const auto &ip = static_cast<const integer_parameter &>(p);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        doc.append(bsoncxx::builder::basic::kvp("type", p.get_type()));
        if (ip.get_min() != std::numeric_limits<long>::min())
            doc.append(bsoncxx::builder::basic::kvp("min", ip.get_min()));
        if (ip.get_max() != std::numeric_limits<long>::max())
            doc.append(bsoncxx::builder::basic::kvp("max", ip.get_max()));
        return doc;
    }
    std::unique_ptr<coco_parameter> integer_parameter_converter::from_bson(const bsoncxx::v_noabi::document::view &doc) const
    {
        auto name = doc["name"].get_string().value.to_string();
        auto min = doc["min"].get_int64().value;
        auto max = doc["max"].get_int64().value;
        return std::make_unique<integer_parameter>(name, min, max);
    }

    bsoncxx::builder::basic::document real_parameter_converter::to_bson(const coco_parameter &p) const
    {
        const auto &rp = static_cast<const real_parameter &>(p);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        doc.append(bsoncxx::builder::basic::kvp("type", p.get_type()));
        if (rp.get_min() != -std::numeric_limits<double>::infinity())
            doc.append(bsoncxx::builder::basic::kvp("min", rp.get_min()));
        if (rp.get_max() != std::numeric_limits<double>::infinity())
            doc.append(bsoncxx::builder::basic::kvp("max", rp.get_max()));
        return doc;
    }
    std::unique_ptr<coco_parameter> real_parameter_converter::from_bson(const bsoncxx::v_noabi::document::view &doc) const
    {
        auto name = doc["name"].get_string().value.to_string();
        auto min = doc["min"].get_double().value;
        auto max = doc["max"].get_double().value;
        return std::make_unique<real_parameter>(name, min, max);
    }

    bsoncxx::builder::basic::document boolean_parameter_converter::to_bson(const coco_parameter &p) const
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        doc.append(bsoncxx::builder::basic::kvp("type", p.get_type()));
        return doc;
    }
    std::unique_ptr<coco_parameter> boolean_parameter_converter::from_bson(const bsoncxx::v_noabi::document::view &doc) const
    {
        auto name = doc["name"].get_string().value.to_string();
        return std::make_unique<boolean_parameter>(name);
    }

    bsoncxx::builder::basic::document symbolic_parameter_converter::to_bson(const coco_parameter &p) const
    {
        const auto &sp = static_cast<const symbolic_parameter &>(p);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        doc.append(bsoncxx::builder::basic::kvp("type", p.get_type()));
        if (!sp.get_symbols().empty())
        {
            bsoncxx::v_noabi::builder::basic::array values;
            for (const auto &value : sp.get_symbols())
                values.append(value);
            doc.append(bsoncxx::builder::basic::kvp("symbols", values));
        }
        return doc;
    }
    std::unique_ptr<coco_parameter> symbolic_parameter_converter::from_bson(const bsoncxx::v_noabi::document::view &doc) const
    {
        auto name = doc["name"].get_string().value.to_string();
        auto symbols = doc["symbols"].get_array().value;
        std::vector<std::string> symbol_list;
        for (auto symbol : symbols)
            symbol_list.push_back(symbol.get_string().value.to_string());
        return std::make_unique<symbolic_parameter>(name, symbol_list);
    }

    bsoncxx::builder::basic::document string_parameter_converter::to_bson(const coco_parameter &p) const
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        doc.append(bsoncxx::builder::basic::kvp("type", p.get_type()));
        return doc;
    }
    std::unique_ptr<coco_parameter> string_parameter_converter::from_bson(const bsoncxx::v_noabi::document::view &doc) const
    {
        auto name = doc["name"].get_string().value.to_string();
        return std::make_unique<string_parameter>(name);
    }

    bsoncxx::builder::basic::document array_parameter_converter::to_bson(const coco_parameter &p) const
    {
        const auto &ap = static_cast<const array_parameter &>(p);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        doc.append(bsoncxx::builder::basic::kvp("type", p.get_type()));
        doc.append(bsoncxx::builder::basic::kvp("array_type", bson_parameter_converter::to_bson(db, ap.as_array_type())));
        bsoncxx::v_noabi::builder::basic::array dimensions;
        for (const auto &dim : ap.get_shape())
            dimensions.append(dim);
        doc.append(bsoncxx::builder::basic::kvp("dimensions", dimensions));
        return doc;
    }
    std::unique_ptr<coco_parameter> array_parameter_converter::from_bson(const bsoncxx::v_noabi::document::view &doc) const
    {
        auto name = doc["name"].get_string().value.to_string();
        auto array_type = bson_parameter_converter::from_bson(db, doc["array_type"].get_document().view());
        std::vector<int> dimensions;
        for (const auto &dim : doc["dimensions"].get_array().value)
            dimensions.push_back(dim.get_int32().value);
        return std::make_unique<array_parameter>(name, std::move(array_type), dimensions);
    }
} // namespace coco
