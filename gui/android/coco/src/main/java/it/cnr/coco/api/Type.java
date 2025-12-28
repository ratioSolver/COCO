package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;

import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;

public class Type {

    private final String name;
    JsonElement data;
    Map<String, Property> static_properties;
    Map<String, Property> dynamic_properties;
    final Collection<Item> instances = new HashSet<>();

    public Type(@NonNull String name, JsonElement data, Map<String, Property> static_properties,
            Map<String, Property> dynamic_properties) {
        this.name = name;
        this.data = data;
        this.static_properties = static_properties;
        this.dynamic_properties = dynamic_properties;
    }

    public String getName() {
        return name;
    }

    public JsonElement getData() {
        return data;
    }

    public Map<String, Property> getStaticProperties() {
        return Collections.unmodifiableMap(static_properties);
    }

    public Map<String, Property> getDynamicProperties() {
        return Collections.unmodifiableMap(dynamic_properties);
    }

    public Collection<Item> getInstances() {
        return Collections.unmodifiableCollection(instances);
    }
}
