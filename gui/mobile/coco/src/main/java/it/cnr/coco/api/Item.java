package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class Item {

    private final String id;
    private final Set<Type> types;
    JsonElement properties;
    private Value value;
    private final List<Value> data = new ArrayList<>();

    public Item(@NonNull String id, @NonNull Set<Type> types, JsonElement properties, Value value) {
        this.id = id;
        this.types = types;
        this.properties = properties;
        this.value = value;
        if (value != null)
            this.data.add(value);
        for (Type type : types)
            type.instances.add(this);
    }

    public String getId() {
        return id;
    }

    public Set<Type> getTypes() {
        return Collections.unmodifiableSet(types);
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
