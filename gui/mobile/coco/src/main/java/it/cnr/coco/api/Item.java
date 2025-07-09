package it.cnr.coco.api;

import androidx.annotation.NonNull;
import it.cnr.coco.api.Type;
import com.google.gson.JsonElement;

public class Item {

    private final String id;
    private final Type type;
    private JsonElement properties;

    public Item(@NonNull String id, @NonNull Type type, JsonElement properties) {
        this.id = id;
        this.type = type;
        this.properties = properties;
    }

    public String getId() {
        return id;
    }

    public Type getType() {
        return type;
    }

    public JsonElement getProperties() {
        return properties;
    }

    public void setProperties(JsonElement properties) {
        this.properties = properties;
    }
}
