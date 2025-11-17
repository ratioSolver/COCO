package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class StringPropertyType implements PropertyType {

    public StringPropertyType() {
    }

    @Override
    public String getName() {
        return "string";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        boolean multiple = obj.has("multiple") && obj.get("multiple").getAsBoolean();
        String defaultValue = obj.has("default") ? obj.get("default").getAsString() : null;
        return new StringProperty(multiple, defaultValue);
    }
}
