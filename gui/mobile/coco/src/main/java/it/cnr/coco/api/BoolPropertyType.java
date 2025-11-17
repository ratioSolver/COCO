package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class BoolPropertyType implements PropertyType {

    public BoolPropertyType() {
    }

    @Override
    public String getName() {
        return "bool";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        boolean multiple = obj.has("multiple") && obj.get("multiple").getAsBoolean();
        Boolean defaultValue = obj.has("default") ? obj.get("default").getAsBoolean() : null;
        return new BoolProperty(multiple, defaultValue);
    }
}
