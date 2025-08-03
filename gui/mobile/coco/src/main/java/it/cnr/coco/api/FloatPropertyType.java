package it.cnr.coco.api;

import java.util.Map;

public class FloatPropertyType implements PropertyType {

    public FloatPropertyType() {
    }

    @Override
    public String getName() {
        return "float";
    }

    @Override
    public Property createProperty(Map<String, Object> property) {
        Double defaultValue = (Double) property.get("default");
        Double min = (Double) property.get("min");
        Double max = (Double) property.get("max");
        return new FloatProperty(defaultValue, min, max);
    }
}
