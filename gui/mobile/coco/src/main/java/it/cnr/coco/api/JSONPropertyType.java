package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

public class JSONPropertyType implements PropertyType {

    public JSONPropertyType() {
    }

    @Override
    public String getName() {
        return "json";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property) {
        Map<String, Object> schema = (Map<String, Object>) property.get("schema");
        Map<String, Object> defaultValue = (Map<String, Object>) property.get("default");
        return new JSONProperty(schema, defaultValue);
    }
}
