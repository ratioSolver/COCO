package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class ItemPropertyType implements PropertyType {

    public ItemPropertyType() {
    }

    @Override
    public String getName() {
        return "item";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        boolean multiple = obj.has("multiple") ? obj.get("multiple").getAsBoolean() : false;
        Type domain = coco.getType(obj.get("domain").getAsString());
        Item defaultValue = obj.has("default") ? coco.getItem(obj.get("default").getAsString()) : null;
        return new ItemProperty(multiple, domain, defaultValue);
    }
}
