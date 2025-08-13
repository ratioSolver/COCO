package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class IntPropertyType implements PropertyType {

    public IntPropertyType() {
    }

    @Override
    public String getName() {
        return "int";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        boolean multiple = obj.has("multiple") && obj.get("multiple").getAsBoolean();
        long min = obj.has("min") ? obj.get("min").getAsLong() : Long.MIN_VALUE;
        long max = obj.has("max") ? obj.get("max").getAsLong() : Long.MAX_VALUE;
        Long defaultValue = obj.has("default") ? obj.get("default").getAsLong() : null;
        return new IntProperty(multiple, min, max, defaultValue);
    }
}
