package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Item {

    private final String id;
    private final Type type;
    JsonElement properties;
    private Value value;
    private final List<Value> data = new ArrayList<>();

    public Item(@NonNull String id, @NonNull Type type, JsonElement properties, Value value) {
        this.id = id;
        this.type = type;
        this.properties = properties;
        this.value = value;
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

    public Value getValue() {
        return value;
    }

    void setValue(@NonNull Value value) {
        this.value = value;
        this.data.add(value);
    }

    public List<Value> getData() {
        return Collections.unmodifiableList(data);
    }
}
