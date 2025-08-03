package it.cnr.coco.api;

import java.util.Map;

public class BoolPropertyType implements PropertyType {

    public BoolPropertyType() {
    }

    @Override
    public String getName() {
        return "bool";
    }

    @Override
    public Property createProperty(Map<String, Object> property) {
        Boolean defaultValue = (Boolean) property.getOrDefault("default", false);
        return new BoolProperty(defaultValue);
    }
}
