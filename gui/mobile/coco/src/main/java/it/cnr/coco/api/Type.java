package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;

import java.util.Collection;
import java.util.HashSet;
import java.util.Map;

public class Type {

    private final String name;
    private Map<String, Type> parents;
    private JsonElement data;
    private Map<String, Property> static_properties;
    private Map<String, Property> dynamic_properties;
    private final Collection<Item> instances = new HashSet<>();

    public Type(@NonNull String name, Map<String, Type> parents, JsonElement data,
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

    public Map<String, Type> getParents() {
        return parents;
    }

    public void setParents(Map<String, Type> parents) {
        this.parents = parents;
    }

    public JsonElement getData() {
        return data;
    }

    public void setData(JsonElement data) {
        this.data = data;
    }

    public Map<String, Property> getStaticProperties() {
        return static_properties;
    }

    public void setStaticProperties(Map<String, Property> static_properties) {
        this.static_properties = static_properties;
    }

    public Map<String, Property> getDynamicProperties() {
        return dynamic_properties;
    }

    public void setDynamicProperties(Map<String, Property> dynamic_properties) {
        this.dynamic_properties = dynamic_properties;
    }

    public Collection<Item> getInstances() {
        return instances;
    }
}
