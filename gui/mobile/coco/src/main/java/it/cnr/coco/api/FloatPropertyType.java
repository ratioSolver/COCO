package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

public class FloatPropertyType implements PropertyType {

    public FloatPropertyType() {
    }

    @Override
    public String getName() {
        return "float";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property) {
        boolean multiple = (boolean) property.getOrDefault("multiple", false);
        double min = (double) property.getOrDefault("min", Double.NEGATIVE_INFINITY);
        double max = (double) property.getOrDefault("max", Double.POSITIVE_INFINITY);
        Double defaultValue = (Double) property.get("default");
        return new FloatProperty(multiple, min, max, defaultValue);
    }
}
