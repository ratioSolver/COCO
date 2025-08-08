package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

public class IntPropertyType implements PropertyType {

    public IntPropertyType() {
    }

    @Override
    public String getName() {
        return "int";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property) {
        boolean multiple = (boolean) property.getOrDefault("multiple", false);
        long min = (long) property.getOrDefault("min", Long.MIN_VALUE);
        long max = (long) property.getOrDefault("max", Long.MAX_VALUE);
        Long defaultValue = (Long) property.get("default");
        return new IntProperty(multiple, min, max, defaultValue);
    }
}
