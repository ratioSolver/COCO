package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

class SymbolPropertyType implements PropertyType {

    public SymbolPropertyType() {
    }

    @Override
    public String getName() {
        return "symbol";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property) {
        JsonObject obj = property.getAsJsonObject();
        String[] defaultValue = null;
        if (obj.has("default"))
            if (obj.get("default").isJsonArray()) {
                JsonArray defaultArray = obj.get("default").getAsJsonArray();
                defaultValue = new String[defaultArray.size()];
                for (int i = 0; i < defaultArray.size(); i++)
                    defaultValue[i] = defaultArray.get(i).getAsString();
            } else
                defaultValue = new String[] { obj.get("default").getAsString() };
        boolean multiple = obj.has("multiple") ? obj.get("multiple").getAsBoolean() : false;
        String[] values = null;
        if (obj.has("values"))
            if (obj.get("values").isJsonArray()) {
                JsonArray valuesArray = obj.get("values").getAsJsonArray();
                values = new String[valuesArray.size()];
                for (int i = 0; i < valuesArray.size(); i++)
                    values[i] = valuesArray.get(i).getAsString();
            } else
                values = new String[] { obj.get("values").getAsString() };
        return new SymbolProperty(multiple, values, defaultValue);
    }
}