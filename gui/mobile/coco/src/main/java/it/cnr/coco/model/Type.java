package it.cnr.coco.model;

import com.google.gson.JsonElement;
import java.util.Map;
import it.cnr.coco.model.Property;

public class Type {

    private String name;
    private JsonElement data;
    private Map<String, Property> static_properties;
    private Map<String, Property> dynamic_properties;

    public Type() {
    }

    public Type(String name, JsonElement data, Map<String, Property> static_properties,
            Map<String, Property> dynamic_properties) {
        this.name = name;
        this.data = data;
        this.static_properties = static_properties;
        this.dynamic_properties = dynamic_properties;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
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
}
