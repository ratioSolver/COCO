package it.cnr.coco.api;

import java.util.Map;

public class IntPropertyType implements PropertyType {

    public IntPropertyType() {
    }

    @Override
    public String getName() {
        return "int";
    }

    @Override
    public Property createProperty(Map<String, Object> property) {
        Long defaultValue = (Long) property.get("default");
        Long min = (Long) property.get("min");
        Long max = (Long) property.get("max");
        return new IntProperty(defaultValue, min, max);
    }
}
