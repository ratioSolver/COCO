package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class JSONPropertyType implements PropertyType {

    public JSONPropertyType() {
    }

    @Override
    public String getName() {
        return "json";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        return new JSONProperty(obj.get("schema"), obj.get("default"));
    }
}
