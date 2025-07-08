package it.cnr.coco.api;

import com.google.gson.JsonElement;

public class Item {

    private String id;
    private String type;
    private JsonElement properties;

    public Item() {
    }

    public Item(String id, String type, JsonElement properties) {
        this.id = id;
        this.type = type;
        this.properties = properties;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public JsonElement getProperties() {
        return properties;
    }

    public void setProperties(JsonElement properties) {
        this.properties = properties;
    }
}
