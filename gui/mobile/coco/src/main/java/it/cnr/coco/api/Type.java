package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;

import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;

public class Type {

    private final String name;
    private Collection<Type> parents;
    private JsonElement data;
    private Map<String, Property> static_properties;
    private Map<String, Property> dynamic_properties;
    private final Collection<Item> instances = new HashSet<>();

    public Type(@NonNull String name, Collection<Type> parents, JsonElement data,
            Map<String, Property> static_properties,
            Map<String, Property> dynamic_properties) {
        this.name = name;
        this.parents = parents;
        this.data = data;
        this.static_properties = static_properties;
        this.dynamic_properties = dynamic_properties;
    }

    public String getName() {
        return name;
    }

    public Collection<Type> getParents() {
        return parents;
    }

    public void setParents(Collection<Type> parents) {
        this.parents = parents;
    }

    public JsonElement getData() {
        return data;
    }

    void setData(JsonElement data) {
        this.data = data;
    }

    public Map<String, Property> getStaticProperties() {
        return Collections.unmodifiableMap(static_properties);
    }

    void setStaticProperties(Map<String, Property> static_properties) {
        this.static_properties = static_properties;
    }

    public Map<String, Property> getDynamicProperties() {
        return Collections.unmodifiableMap(dynamic_properties);
    }

    void setDynamicProperties(Map<String, Property> dynamic_properties) {
        this.dynamic_properties = dynamic_properties;
    }

    public Collection<Item> getInstances() {
        return Collections.unmodifiableCollection(instances);
    }
}
