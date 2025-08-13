package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class FloatPropertyType implements PropertyType {

    public FloatPropertyType() {
    }

    @Override
    public String getName() {
        return "float";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        boolean multiple = obj.has("multiple") ? obj.get("multiple").getAsBoolean() : false;
        double min = obj.has("min") ? obj.get("min").getAsDouble() : Double.NEGATIVE_INFINITY;
        double max = obj.has("max") ? obj.get("max").getAsDouble() : Double.POSITIVE_INFINITY;
        Double defaultValue = obj.has("default") ? obj.get("default").getAsDouble() : null;
        return new FloatProperty(multiple, min, max, defaultValue);
    }
}
