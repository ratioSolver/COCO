package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

public class ItemPropertyType implements PropertyType {

    public ItemPropertyType() {
    }

    @Override
    public String getName() {
        return "item";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property) {
        boolean multiple = (boolean) property.getOrDefault("multiple", false);
        Type domain = coco.getType((String) property.get("domain"));
        Item defaultValue = property.containsKey("default") ? coco.getItem((String) property.get("default")) : null;
        return new ItemProperty(multiple, domain, defaultValue);
    }
}
