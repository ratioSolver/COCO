package it.cnr.coco.api;

import java.util.Map;

public class JSONProperty implements Property {

    private final Map<String, Object> schema;
    private final Map<String, Object> defaultValue;

    public JSONProperty(Map<String, Object> schema, Map<String, Object> defaultValue) {
        this.schema = schema;
        this.defaultValue = defaultValue;
    }

    public Map<String, Object> getSchema() {
        return schema;
    }

    public Map<String, Object> getDefaultValue() {
        return defaultValue;
    }
}
