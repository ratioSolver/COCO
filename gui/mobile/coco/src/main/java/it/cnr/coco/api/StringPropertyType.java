package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

public class StringPropertyType implements PropertyType {

    public StringPropertyType() {
    }

    @Override
    public String getName() {
        return "string";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property) {
        boolean multiple = (boolean) property.getOrDefault("multiple", false);
        String defaultValue = (String) property.get("default");
        return new StringProperty(multiple, defaultValue);
    }
}
