#include "coco_server.hpp"
#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#ifdef BUILD_AUTH
#include "coco_auth.hpp"
#else
#include "coco_noauth.hpp"
#endif
#include "logging.hpp"

namespace coco
{
    server_module::server_module(coco_server &srv) noexcept : srv(srv) {}
    coco &server_module::get_coco() noexcept { return srv.get_coco(); }

    void server_module::add_schema(std::string_view name, json::json &&schema) noexcept { srv.schemas[name] = std::move(schema); }
    json::json &server_module::get_schema(std::string_view name)
    {
        if (srv.schemas.as_object().count(name.data()))
            return srv.schemas[name];
        throw std::invalid_argument("Schema not found: " + std::string(name));
    }
    void server_module::add_path(std::string_view path, json::json &&path_info) noexcept { srv.paths[path.data()] = std::move(path_info); }
    json::json &server_module::get_path(std::string_view path)
    {
        if (srv.paths.as_object().count(path.data()))
            return srv.paths[path];
        throw std::invalid_argument("Path not found: " + std::string(path));
    }

#ifdef BUILD_SECURE
    coco_server::coco_server(coco &cc, std::string_view host, unsigned short port) : coco_module(cc), listener(cc), ssl_server(host, port)
#else
    coco_server::coco_server(coco &cc, std::string_view host, unsigned short port) : coco_module(cc), listener(cc), server(host, port)
#endif
    {
        add_route(network::Get, "^/$", std::bind(&coco_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.jpg|/.+\\.png|/.+\\.gif", std::bind(&coco_server::assets, this, network::placeholders::request));

        add_route(network::Get, "^/types$", std::bind(&coco_server::get_types, this, network::placeholders::request));
        add_route(network::Get, "^/types/.*$", std::bind(&coco_server::get_type, this, network::placeholders::request));
        add_route(network::Post, "^/types$", std::bind(&coco_server::create_type, this, network::placeholders::request));
        add_route(network::Delete, "^/types/.*$", std::bind(&coco_server::delete_type, this, network::placeholders::request));

        add_route(network::Get, "^/items(\\?([a-zA-Z0-9_\\-]+=[^&=#]+)(\\&[a-zA-Z0-9_\\-]+=[^&=#]+)*)?$", std::bind(&coco_server::get_items, this, network::placeholders::request));
        add_route(network::Get, "^/items/.*$", std::bind(&coco_server::get_item, this, network::placeholders::request));
        add_route(network::Post, "^/items$", std::bind(&coco_server::create_item, this, network::placeholders::request));
        add_route(network::Patch, "^/items/.*$", std::bind(&coco_server::update_item, this, network::placeholders::request));
        add_route(network::Delete, "^/items/.*$", std::bind(&coco_server::delete_item, this, network::placeholders::request));

        add_route(network::Get, "^/data/.*$", std::bind(&coco_server::get_data, this, network::placeholders::request));
        add_route(network::Post, "^/data/.*$", std::bind(&coco_server::set_datum, this, network::placeholders::request));

        add_route(network::Get, "^/fake/.*$", std::bind(&coco_server::fake, this, network::placeholders::request));

        add_route(network::Get, "^/reactive_rules$", std::bind(&coco_server::get_reactive_rules, this, network::placeholders::request));
        add_route(network::Post, "^/reactive_rules$", std::bind(&coco_server::create_reactive_rule, this, network::placeholders::request));

        add_route(network::Get, "^/openapi$", std::bind(&coco_server::get_openapi_spec, this, network::placeholders::request));
        add_route(network::Get, "^/asyncapi$", std::bind(&coco_server::get_openapi_spec, this, network::placeholders::request));

        add_ws_route("/coco").on_open(std::bind(&coco_server::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&coco_server::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&coco_server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&coco_server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));

        schemas["property"] = {
            {"description", "A property definition that can be one of several types: integer, float, string, symbol, item reference, or JSON object."},
            {"oneOf", std::vector<json::json>{{"$ref", "#/components/schemas/int_property"}, {"$ref", "#/components/schemas/float_property"}, {"$ref", "#/components/schemas/string_property"}, {"$ref", "#/components/schemas/symbol_property"}, {"$ref", "#/components/schemas/item_property"}, {"$ref", "#/components/schemas/json_property"}}}};
        schemas["int_property"] = {
            {"type", "object"},
            {"description", "A property that holds integer values, with optional constraints and default values."},
            {"properties",
             {{"type", {{"type", "string"}, {"enum", {"int"}}, {"description", "The property type identifier."}}},
              {"nullable", {{"type", "boolean"}, {"description", "Whether this property can be null."}}},
              {"multiple", {{"type", "boolean"}, {"description", "Whether this property can hold multiple values (array)."}}},
              {"default", {{"oneOf", std::vector<json::json>{{{"type", "integer"}}, {{"type", "array"}, {"items", {{"type", "integer"}}}}}}, {"description", "Default value(s) for this property."}}},
              {"min", {{"type", "integer"}, {"description", "Minimum allowed value for this property."}}},
              {"max", {{"type", "integer"}, {"description", "Maximum allowed value for this property."}}}}},
            {"required", std::vector<json::json>{"type"}}};
        schemas["float_property"] = {
            {"type", "object"},
            {"description", "A property that holds floating-point numbers, with optional constraints and default values."},
            {"properties",
             {{"type", {{"type", "string"}, {"enum", {"float"}}, {"description", "The property type identifier."}}},
              {"nullable", {{"type", "boolean"}, {"description", "Whether this property can be null."}}},
              {"multiple", {{"type", "boolean"}, {"description", "Whether this property can hold multiple values (array)."}}},
              {"default", {{"oneOf", std::vector<json::json>{{{"type", "number"}}, {{"type", "array"}, {"items", {{"type", "number"}}}}}}, {"description", "Default value(s) for this property."}}},
              {"min", {{"type", "number"}, {"description", "Minimum allowed value for this property."}}},
              {"max", {{"type", "number"}, {"description", "Maximum allowed value for this property."}}}}},
            {"required", std::vector<json::json>{"type"}}};
        schemas["string_property"] = {
            {"type", "object"},
            {"description", "A property that holds string values, with optional default values."},
            {"properties",
             {{"type", {{"type", "string"}, {"enum", {"string"}}, {"description", "The property type identifier."}}},
              {"nullable", {{"type", "boolean"}, {"description", "Whether this property can be null."}}},
              {"multiple", {{"type", "boolean"}, {"description", "Whether this property can hold multiple values (array)."}}},
              {"default", {{"oneOf", std::vector<json::json>{{{"type", "string"}}, {{"type", "array"}, {"items", {{"type", "string"}}}}}}, {"description", "Default value(s) for this property."}}}}},
            {"required", std::vector<json::json>{"type"}}};
        schemas["symbol_property"] = {
            {"type", "object"},
            {"description", "A property that holds predefined symbolic values from a restricted set of allowed values."},
            {"properties",
             {{"type", {{"type", "string"}, {"enum", {"symbol"}}, {"description", "The property type identifier."}}},
              {"nullable", {{"type", "boolean"}, {"description", "Whether this property can be null."}}},
              {"values", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "The allowed symbolic values for this property."}}},
              {"multiple", {{"type", "boolean"}, {"description", "Whether this property can hold multiple values (array)."}}},
              {"default", {{"oneOf", std::vector<json::json>{{{"type", "string"}}, {{"type", "array"}, {"items", {{"type", "string"}}}}}}, {"description", "Default value(s) for this property."}}}}},
            {"required", std::vector<json::json>{"type"}}};
        schemas["item_property"] = {
            {"type", "object"},
            {"description", "A property that holds references to other items by their ID, restricted to a specific domain type."},
            {"properties",
             {{"type", {{"type", "string"}, {"enum", {"item"}}, {"description", "The property type identifier."}}},
              {"nullable", {{"type", "boolean"}, {"description", "Whether this property can be null."}}},
              {"domain", {{"type", "string"}, {"description", "The type of objects that are allowed as values for this property."}}},
              {"multiple", {{"type", "boolean"}, {"description", "Whether this property can hold multiple values (array)."}}},
              {"default", {{"oneOf", std::vector<json::json>{{{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}, {{"type", "array"}, {"items", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}}}}}, {"description", "Default ID value(s) for this property."}}}}},
            {"required", std::vector<json::json>{"type", "domain"}}};
        schemas["json_property"] = {
            {"type", "object"},
            {"description", "A property that holds arbitrary JSON objects conforming to a specified JSON schema."},
            {"properties",
             {{"type", {{"type", "string"}, {"enum", {"json"}}, {"description", "The property type identifier."}}},
              {"nullable", {{"type", "boolean"}, {"description", "Whether this property can be null."}}},
              {"schema", {{"type", "object"}, {"description", "The JSON schema that validates the property's value."}}},
              {"default", {{"type", "object"}, {"description", "Default JSON object for this property."}}}}},
            {"required", std::vector<json::json>{"type", "schema"}}};
        schemas["type"] = {
            {"type", "object"},
            {"description", "A " COCO_NAME " type definition that describes the structure and behavior of items."},
            {"properties",
             {{"name", {{"type", "string"}, {"description", "The unique name identifier for this type."}}},
              {"static_properties", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/property"}}}, {"description", "Object containing static properties that define the fixed structure of items of this type. Keys are property names, values are property definitions."}}},
              {"dynamic_properties", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/property"}}}, {"description", "Object containing dynamic properties that can store time-series data for items of this type. Keys are property names, values are property definitions."}}},
              {"data", {{"type", "object"}, {"description", "Additional metadata or configuration data for this type."}}}}},
            {"required", std::vector<json::json>{"name"}}};
        schemas["item"] = {
            {"type", "object"},
            {"description", "A " COCO_NAME " item is an instance of a type, which can have static properties and dynamic data."},
            {"properties",
             {{"id", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}, {"description", "The ID for this item."}}},
              {"type", {{"type", "string"}, {"description", "The name of the type that this item instantiates."}}},
              {"properties", {{"type", "object"}, {"description", "Static data of the item defined by its type."}}},
              {"value", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/data"}}}, {"description", "Dynamic data of the item defined by its type."}}}}},
            {"required", std::vector<json::json>{"id", "type"}}};
        schemas["data"] = {
            {"type", "object"},
            {"description", "A data entry containing dynamic values and associated metadata for an item."},
            {"properties",
             {{"data", {{"type", "object"}, {"description", "Dynamic data of the item defined by its type."}}},
              {"timestamp", {{"type", "integer"}, {"format", "int64"}, {"description", "Unix timestamp in milliseconds when this data was recorded."}}}}},
            {"required", std::vector<json::json>{"data", "timestamp"}}};
        schemas["reactive_rule"] = {
            {"type", "object"},
            {"description", "A reactive rule is a CLIPS rule that can be triggered by changes in the system."},
            {"properties",
             {{"name", {{"type", "string"}}},
              {"content", {{"type", "string"}, {"description", "The content of the reactive rule in CLIPS format."}}}}},
            {"required", std::vector<json::json>{"name", "content"}}};

        paths["/types"] = {{"get",
                            {{"summary", "Retrieve all the " COCO_NAME " types."},
                             {"description", "Endpoint to fetch all the managed types."},
#ifdef BUILD_AUTH
                             {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                             {"responses",
                              {{"200",
                                {{"description", "Successful response containing an array of all managed types with their complete definitions."},
                                 {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/type"}}}}}}}}}}}
#ifdef BUILD_AUTH
                               ,
                               {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}}
#endif
                              }}}},
                           {"post",
                            {{"summary", "Create a new " COCO_NAME " type."},
                             {"description", "Endpoint to create a new type."},
                             {"requestBody",
                              {{"required", true},
                               {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/type"}}}}}}}}},
#ifdef BUILD_AUTH
                             {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                             {"responses",
                              {{"204",
                                {{"description", "Type created successfully."}}}}}}}};
        paths["/types/{name}"] = {{"get",
                                   {{"summary", "Retrieve a specific " COCO_NAME " type."},
                                    {"description", "Endpoint to fetch a specific type by name."},
                                    {"parameters",
                                     {{{"name", "name"}, {"description", "The name of the specific " COCO_NAME " type to get."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}}},
#ifdef BUILD_AUTH
                                    {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                    {"responses",
                                     {{"200",
                                       {{"description", "Successful response with the type details."},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/type"}}}}}}}}},
#ifdef BUILD_AUTH
                                      {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                      {"404",
                                       {{"description", "Type not found"}}}}}}},
                                  {"delete",
                                   {{"summary", "Delete a specific " COCO_NAME " type."},
                                    {"description", "Endpoint to delete a specific type by name."},
                                    {"parameters",
                                     {{{"name", "name"}, {"description", "The name of the specific " COCO_NAME " type to delete."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}}},
#ifdef BUILD_AUTH
                                    {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                    {"responses",
                                     {{"204",
                                       {{"description", "Type deleted successfully"}}},
#ifdef BUILD_AUTH
                                      {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                      {"404",
                                       {{"description", "Type not found"}}}}}}}};
        paths["/items"] = {{"get",
                            {{"summary", "Retrieve all the " COCO_NAME " items."},
                             {"description", "Endpoint to fetch all the managed items. You can filter items by type and other properties using query parameters."},
                             {"parameters",
                              {{{"name", "type"}, {"description", "Filter items by type name."}, {"in", "query"}, {"required", false}, {"schema", {{"type", "string"}}}},
                               {{"name", "types"}, {"description", "Filter items by multiple type names (comma-separated)."}, {"in", "query"}, {"required", false}, {"schema", {{"type", "string"}}}},
                               {{"name", "\"\""}, {"description", "Filter items by specific properties."}, {"in", "query"}, {"required", false}, {"style", "form"}, {"explode", true}, {"schema", {{"type", "object"}, {"additionalProperties", {{"type", "string"}}}}}}}},
#ifdef BUILD_AUTH
                             {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                             {"responses",
                              {{"200",
                                {{"description", "Successful response containing an array of all managed items with their properties and metadata."},
                                 {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/item"}}}}}}}}}}},
#ifdef BUILD_AUTH
                               {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                               {"404",
                                {{"description", "Type not found"}}}}}}},
                           {"post",
                            {{"summary", "Create a new " COCO_NAME " item."},
                             {"description", "Endpoint to create a new item."},
                             {"requestBody",
                              {{"required", true},
                               {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/item"}}}}}}}}},
#ifdef BUILD_AUTH
                             {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                             {"responses",
                              {{"201",
                                {{"description", "Item created successfully."},
                                 {"content", {{"text/plain", {{"schema", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}, {"description", "The ID of the newly created item."}}}}}}}}}
#ifdef BUILD_AUTH
                               ,
                               {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}}
#endif
                              }}}}};
        paths["/items/{id}"] = {{"get",
                                 {{"summary", "Retrieve a specific " COCO_NAME " item."},
                                  {"description", "Endpoint to fetch a specific item by ID."},
                                  {"parameters",
                                   {{{"name", "id"}, {"description", "The ID of the specific " COCO_NAME " item to get."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}}}}},
                                  {"responses",
                                   {{"200",
                                     {{"description", "Successful response with the item details."},
                                      {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/item"}}}}}}}}},
#ifdef BUILD_AUTH
                                    {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                    {"404",
                                     {{"description", "Item not found"}}}}}}},
                                {"patch",
                                 {{"summary", "Update a specific " COCO_NAME " item."},
                                  {"description", "Endpoint to update a specific item by ID. You can provide partial updates for the item's properties."},
                                  {"parameters",
                                   {{{"name", "id"}, {"description", "The ID of the specific " COCO_NAME " item to update."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}}}}},
                                  {"requestBody",
                                   {{"required", true},
                                    {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/item"}}}}}}}}},
#ifdef BUILD_AUTH
                                  {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                  {"responses",
                                   {{"204",
                                     {{"description", "Item updated successfully."}}},
#ifdef BUILD_AUTH
                                    {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                    {"404",
                                     {{"description", "Item not found"}}}}}}},
                                {"delete",
                                 {{"summary", "Delete a specific " COCO_NAME " item."},
                                  {"description", "Endpoint to delete a specific item by ID."},
                                  {"parameters",
                                   {{{"name", "id"}, {"description", "The ID of the specific " COCO_NAME " item to delete."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}}}}},
#ifdef BUILD_AUTH
                                  {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                  {"responses",
                                   {{"204",
                                     {{"description", "Item deleted successfully"}}},
#ifdef BUILD_AUTH
                                    {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                    {"404",
                                     {{"description", "Item not found"}}}}}}}};
        paths["/data/{id}"] = {{"get",
                                {{"summary", "Retrieve data for a specific " COCO_NAME " item."},
                                 {"description", "Endpoint to fetch data for a specific item by ID. You can filter data by providing 'from' and 'to' query parameters."},
                                 {"parameters",
                                  {{{"name", "id"}, {"description", "The ID of the " COCO_NAME " item."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}}},
                                   {{"name", "from"}, {"description", "Start date for filtering data."}, {"in", "query"}, {"schema", {{"type", "string"}, {"format", "date-time"}}}},
                                   {{"name", "to"}, {"description", "End date for filtering data."}, {"in", "query"}, {"schema", {{"type", "string"}, {"format", "date-time"}}}}}},
#ifdef BUILD_AUTH
                                 {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                 {"responses",
                                  {{"200",
                                    {{"description", "Successful response with the item data."},
                                     {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/data"}}}}}}}}}}},
#ifdef BUILD_AUTH
                                   {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                   {"404",
                                    {{"description", "Item not found."}}}}}}},
                               {"post",
                                {{"summary", "Add data to a specific " COCO_NAME " item."},
                                 {"description", "Endpoint to add data to a specific item by ID. You can provide a timestamp for the data."},
                                 {"parameters",
                                  {{{"name", "id"}, {"description", "The ID of the " COCO_NAME " item."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}, {"pattern", "^[a-fA-F0-9]{24}$"}}}},
                                   {{"name", "timestamp"}, {"description", "Timestamp for the data."}, {"in", "query"}, {"schema", {{"type", "string"}, {"format", "date-time"}}}}}},
                                 {"requestBody",
                                  {{"required", true},
                                   {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/data"}}}}}}}}},
#ifdef BUILD_AUTH
                                 {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                 {"responses",
                                  {{"204",
                                    {{"description", "Data added successfully."}}},
#ifdef BUILD_AUTH
                                   {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}},
#endif
                                   {"404",
                                    {{"description", "Item not found."}}}}}}}};
        paths["/fake/{type}"] = {{"get",
                                  {{"summary", "Generate fake data for testing."},
                                   {"description", "Endpoint to generate fake data for testing purposes."},
                                   {"parameters",
                                    {{{"name", "type"}, {"description", "The " COCO_NAME " type of fake data to generate."}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}},
                                     {{"name", "parameters"}, {"description", "The names of the dynamic parameters to generate fake values for. If omitted, all dynamic parameters will be generated."}, {"in", "query"}, {"style", "form"}, {"explode", false}, {"schema", {{"type", "array"}, {"items", {{"type", "string"}}}}}}}},
#ifdef BUILD_AUTH
                                   {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                   {"responses",
                                    {{"200",
                                      {{"description", "Successful response with the generated fake data."},
                                       {"content", {{"application/json", {{"schema", {{"type", "object"}}}}}}}}}}}}}},
        paths["/reactive_rules"] = {{"get",
                                     {{"summary", "Retrieve all the " COCO_NAME " reactive rules."},
                                      {"description", "Endpoint to fetch all the reactive rules."},
#ifdef BUILD_AUTH
                                      {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                      {"responses",
                                       {{"200",
                                         {{"description", "Successful response with the stored reactive rules."},
                                          {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/reactive_rule"}}}}}}}}}}}
#ifdef BUILD_AUTH
                                        ,
                                        {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}}
#endif
                                       }}}},
                                    {"post",
                                     {{"summary", "Create a new " COCO_NAME " reactive rule."},
                                      {"description", "Endpoint to create a new reactive rule."},
                                      {"requestBody",
                                       {{"required", true},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/reactive_rule"}}}}}}}}},
#ifdef BUILD_AUTH
                                      {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                                      {"responses",
                                       {{"204",
                                         {{"description", "Reactive rule created successfully."}}}
#ifdef BUILD_AUTH
                                        ,
                                        {"401", {{"$ref", "#/components/responses/UnauthorizedError"}}}
#endif
                                       }}}}};

#ifdef BUILD_AUTH
        add_module<server_auth>(*this);
        auto &auth_mdwr = add_middleware<auth_middleware>(*this, get_coco());
        auth_mdwr.add_authorized_path(network::Get, "^/types$", {0, 1});
        auth_mdwr.add_authorized_path(network::Post, "^/types$", {0});
        auth_mdwr.add_authorized_path(network::Get, "^/types/.*$", {0, 1});
        auth_mdwr.add_authorized_path(network::Delete, "^/types/.*$", {0});
        auth_mdwr.add_authorized_path(network::Get, "^/items$", {0, 1});
        auth_mdwr.add_authorized_path(network::Post, "^/items$", {0});
        auth_mdwr.add_authorized_path(network::Get, "^/items/.*$", {0, 1}, true);
        auth_mdwr.add_authorized_path(network::Delete, "^/items/.*$", {0});
        auth_mdwr.add_authorized_path(network::Get, "^/data/.*$", {0, 1}, true);
        auth_mdwr.add_authorized_path(network::Post, "^/data/.*$", {0, 1}, true);
        auth_mdwr.add_authorized_path(network::Get, "^/reactive_rules$", {0, 1});
        auth_mdwr.add_authorized_path(network::Post, "^/reactive_rules$", {0});
#else
        add_module<server_noauth>(*this);
#endif
    }

    void coco_server::on_ws_open(network::ws_server_session_base &ws)
    {
        for (auto &[_, mod] : modules)
            mod->on_ws_open(ws);
    }
    void coco_server::on_ws_message(network::ws_server_session_base &ws, const network::message &msg)
    {
        for (auto &[_, mod] : modules)
            mod->on_ws_message(ws, msg);
    }
    void coco_server::on_ws_close(network::ws_server_session_base &ws)
    {
        for (auto &[_, mod] : modules)
            mod->on_ws_close(ws);
    }
    void coco_server::on_ws_error(network::ws_server_session_base &ws, const std::error_code &ec)
    {
        for (auto &[_, mod] : modules)
            mod->on_ws_error(ws, ec);
    }
    void coco_server::broadcast(json::json &&msg)
    {
        for (auto &[_, mod] : modules)
            mod->broadcast(msg);
    }

    void coco_server::created_type(const type &tp)
    {
        auto j_tp = tp.to_json();
        j_tp["msg_type"] = "new_type";
        j_tp["name"] = tp.get_name();
        broadcast(std::move(j_tp));
    }
    void coco_server::created_item(const item &itm)
    {
        auto j_itm = itm.to_json();
        j_itm["msg_type"] = "new_item";
        j_itm["id"] = itm.get_id();
        broadcast(std::move(j_itm));
    }
    void coco_server::updated_item(const item &itm)
    {
        auto j_itm = itm.to_json();
        j_itm["msg_type"] = "updated_item";
        j_itm["id"] = itm.get_id();
        broadcast(std::move(j_itm));
    }
    void coco_server::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) { broadcast({{"msg_type", "new_data"}, {"id", itm.get_id()}, {"value", {{"data", data}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()}}}}); }

    std::unique_ptr<network::response> coco_server::index([[maybe_unused]] const network::request &req) { return std::make_unique<network::file_response>(CLIENT_DIR "/dist/index.html"); }
    std::unique_ptr<network::response> coco_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR "/dist" + target);
    }

    std::unique_ptr<network::response> coco_server::get_types([[maybe_unused]] const network::request &req)
    {
        json::json ts(json::json_type::array);
        for (auto &tp : get_coco().get_types())
        {
            auto j_tp = tp.get().to_json();
            j_tp["name"] = tp.get().get_name();
            ts.push_back(std::move(j_tp));
        }
        return std::make_unique<network::json_response>(std::move(ts));
    }
    std::unique_ptr<network::response> coco_server::get_type(const network::request &req)
    {
        try
        { // get type by name in the path
            auto &tp = get_coco().get_type(req.get_target().substr(7));
            auto j_tp = tp.to_json();
            j_tp["name"] = tp.get_name();
            return std::make_unique<network::json_response>(std::move(j_tp));
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
    }
    std::unique_ptr<network::response> coco_server::create_type(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("name") || !body["name"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

        std::string name = body["name"];

        json::json static_props;
        if (body.contains("static_properties"))
            static_props = std::move(body["static_properties"]);

        json::json dynamic_props;
        if (body.contains("dynamic_properties"))
            dynamic_props = std::move(body["dynamic_properties"]);

        json::json data;
        if (body.contains("data"))
            data = std::move(body["data"]);

        [[maybe_unused]] auto &tp = get_coco().create_type(name, std::move(static_props), std::move(dynamic_props), std::move(data));
        return std::make_unique<network::response>(network::status_code::no_content);
    }
    std::unique_ptr<network::response> coco_server::delete_type(const network::request &req)
    {
        try
        { // get type by name in the path
            get_coco().delete_type(get_coco().get_type(req.get_target().substr(7)));
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_items(const network::request &req)
    {
        std::map<std::string, std::string> filter;
        if (req.get_target().find('?') != std::string::npos)
            filter = network::parse_query(req.get_target().substr(req.get_target().find('?') + 1));
        json::json is(json::json_type::array);
        if (filter.count("type")) // filter by type
            try
            {
                auto &tp = get_coco().get_type(filter["type"]);
                for (auto &itm : get_coco().get_items(tp))
                    if (std::all_of(filter.begin(), filter.end(), [&](const auto &p)
                                    { return p.first == "type" || (itm.get().get_properties().contains(p.first) && itm.get().get_properties()[p.first] == p.second); }))
                    {
                        auto j_itm = itm.get().to_json();
                        j_itm["id"] = itm.get().get_id();
                        is.push_back(std::move(j_itm));
                    }
                return std::make_unique<network::json_response>(std::move(is));
            }
            catch (const std::exception &)
            {
                return std::make_unique<network::json_response>(json::json({{"message", "Type `" + filter["type"] + "` not found"}}), network::status_code::not_found);
            }
        else if (filter.count("types")) // filter by multiple types
        {
            std::vector<std::reference_wrapper<type>> types;
            auto tp_names = network::split_string(filter["types"], ',');
            for (auto &tp_name : tp_names)
                try
                {
                    types.push_back(get_coco().get_type(tp_name));
                }
                catch (const std::exception &)
                {
                    return std::make_unique<network::json_response>(json::json({{"message", "Type `" + tp_name + "` not found"}}), network::status_code::not_found);
                }
            std::unordered_set<std::string> seen_ids;
            for (auto &tp : types)
                for (auto &itm : get_coco().get_items(tp))
                {
                    for (const auto &[par, val] : filter)
                        if (par != "types" && (!itm.get().get_properties().contains(par) || itm.get().get_properties()[par] != val))
                            continue; // skip items that do not match the filter
                    auto j_itm = itm.get().to_json();
                    if (seen_ids.count(itm.get().get_id()) == 0)
                    {
                        j_itm["id"] = itm.get().get_id();
                        is.push_back(std::move(j_itm));
                        seen_ids.insert(itm.get().get_id());
                    }
                }
            return std::make_unique<network::json_response>(std::move(is));
        }
        else
            for (auto &itm : get_coco().get_items())
            {
                for (const auto &[par, val] : filter)
                    if (!itm.get().get_properties().contains(par) || itm.get().get_properties()[par] != val)
                        continue; // skip items that do not match the filter
                auto j_itm = itm.get().to_json();
                j_itm["id"] = itm.get().get_id();
                is.push_back(std::move(j_itm));
            }
        return std::make_unique<network::json_response>(std::move(is));
    }
    std::unique_ptr<network::response> coco_server::get_item(const network::request &req)
    {
        try
        { // get item by id in the path
            auto &itm = get_coco().get_item(req.get_target().substr(7));
            auto j_tp = itm.to_json();
            j_tp["id"] = itm.get_id();
            return std::make_unique<network::json_response>(std::move(j_tp));
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }
    std::unique_ptr<network::response> coco_server::create_item(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

        std::vector<std::reference_wrapper<type>> types;
        if (body.contains("types"))
        {
            if (!body["types"].is_array())
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &tp_name : body["types"].as_array())
            {
                if (!tp_name.is_string())
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                try
                {
                    types.push_back(get_coco().get_type(tp_name.get<std::string>()));
                }
                catch (const std::exception &)
                {
                    return std::make_unique<network::json_response>(json::json({{"message", "Type `" + tp_name.get<std::string>() + "` not found"}}), network::status_code::not_found);
                }
            }
        }
        try
        {
            json::json props = body.contains("properties") ? body["properties"] : json::json();
            auto &itm = get_coco().create_item(std::move(types), std::move(props));
            return std::make_unique<network::string_response>(std::string(itm.get_id()), network::status_code::created);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
    std::unique_ptr<network::response> coco_server::update_item(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

        try
        {
            json::json props = body.contains("properties") ? body["properties"] : json::json();
            auto &itm = get_coco().get_item(req.get_target().substr(7));
            if (body.contains("properties"))
            {
                if (!body["properties"].is_object())
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                get_coco().set_properties(itm, std::move(props));
            }
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }
    std::unique_ptr<network::response> coco_server::delete_item(const network::request &req)
    {
        try
        { // get item by id in the path
            get_coco().delete_item(get_coco().get_item(req.get_target().substr(7)));
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_data(const network::request &req)
    {
        // get item by id in the path
        auto id = req.get_target().substr(6);
        std::map<std::string, std::string> params;
        if (id.find('?') != std::string::npos)
        {
            params = network::parse_query(id.substr(id.find('?') + 1));
            id = id.substr(0, id.find('?'));
        }
        item *itm;
        try
        {
            itm = &get_coco().get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        std::chrono::system_clock::time_point to = params.count("to") ? std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("to"))}) : std::chrono::system_clock::time_point();
        std::chrono::system_clock::time_point from = params.count("from") ? std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("from"))}) : to - std::chrono::hours{24 * 7};
        return std::make_unique<network::json_response>(get_coco().get_values(*itm, from, to));
    }
    std::unique_ptr<network::response> coco_server::set_datum(const network::request &req)
    {
        // get item by id in the path
        auto id = req.get_target().substr(6);
        std::map<std::string, std::string> params;
        if (id.find('?') != std::string::npos)
        {
            params = network::parse_query(id.substr(id.find('?') + 1));
            id = id.substr(0, id.find('?'));
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        try
        {
            auto &itm = get_coco().get_item(id);
            if (params.count("timestamp"))
            {
                std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("timestamp"))});
                get_coco().set_value(itm, json::json(body), timestamp);
            }
            else
                get_coco().set_value(itm, json::json(body));
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::fake(const network::request &req)
    {
        auto name = req.get_target().substr(6);
        std::map<std::string, std::string> params;
        if (name.find('?') != std::string::npos)
        {
            params = network::parse_query(name.substr(name.find('?') + 1));
            name = name.substr(0, name.find('?'));
        }
        type *tp;
        try
        {
            tp = &get_coco().get_type(name);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }

        auto &props = tp->get_dynamic_properties();
        json::json j;

        if (params.count("parameters"))
            try
            {
                const auto pars = json::load(network::decode(params.at("parameters")));
                for (const auto &par : pars.as_array())
                    if (auto it = props.find(par.get<std::string>()); it != props.end())
                        j[it->first] = it->second->fake();
                    else
                        return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            }
            catch (const std::exception &)
            {
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            }
        else // fake all properties
            for (auto &[name, prop] : props)
                j[name] = prop->fake();

        return std::make_unique<network::json_response>(std::move(j));
    }

    std::unique_ptr<network::response> coco_server::get_reactive_rules([[maybe_unused]] const network::request &req)
    {
        json::json is(json::json_type::array);
        for (auto &rr : get_coco().get_reactive_rules())
        {
            auto j_rrs = rr.get().to_json();
            j_rrs["name"] = rr.get().get_name();
            is.push_back(std::move(j_rrs));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }
    std::unique_ptr<network::response> coco_server::create_reactive_rule(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("name") || !body["name"].is_string() || !body.contains("content") || !body["content"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string content = body["content"];
        try
        {
            [[maybe_unused]] auto &rule = get_coco().create_reactive_rule(name, content);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }

    std::unique_ptr<network::response> coco_server::get_openapi_spec([[maybe_unused]] const network::request &req)
    {
        json::json spec = {{"openapi", "3.1.0"},
                           {"info",
                            {{"title", COCO_NAME " Server API"},
                             {"version", "1.0.0"},
                             {"description", "RESTful API for the " COCO_NAME " server. This API provides comprehensive management of types, items, dynamic data, and reactive rules. Types define the structure and behavior of items, items are instances of types with static properties, and dynamic data provides time-series storage capabilities. Reactive rules enable event-driven processing using CLIPS rule engine."}}},
                           {"components",
                            {
#ifdef BUILD_AUTH
                                {"securitySchemes", {"bearerAuth", {{"type", "http"}, {"scheme", "bearer"}}}},
                                {"responses", {"UnauthorizedError", {{"description", "Access token is missing or invalid"}}}},
#endif
                                {"schemas", schemas}}},
                           {"paths", paths}};
        return std::make_unique<network::json_response>(std::move(spec));
    }
    std::unique_ptr<network::response> coco_server::get_asyncapi_spec([[maybe_unused]] const network::request &req)
    {
        json::json spec = {{"asyncapi", "3.0.0"},
                           {"info",
                            {{"title", COCO_NAME " Server API"},
                             {"version", "1.0.0"},
                             {"description", "API for the " COCO_NAME " server."}}},
                           {"channels", {}},
                           {"components",
                            {
#ifdef BUILD_AUTH
                                {"securitySchemes", {"bearerAuth", {{"type", "http"}, {"scheme", "bearer"}}}},
#endif
                                {"schemas", schemas}}}};
        return std::make_unique<network::json_response>(std::move(spec));
    }
} // namespace coco
