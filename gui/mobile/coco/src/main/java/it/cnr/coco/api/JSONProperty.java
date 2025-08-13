package it.cnr.coco.api;

import com.google.gson.JsonElement;

public class JSONProperty implements Property {

    private final JsonElement schema;
    private final JsonElement defaultValue;

    public JSONProperty(JsonElement schema, JsonElement defaultValue) {
        this.schema = schema;
        this.defaultValue = defaultValue;
    }

    public JsonElement getSchema() {
        return schema;
    }

    public JsonElement getDefaultValue() {
        return defaultValue;
    }
}
