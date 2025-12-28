package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import java.time.Instant;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import it.cnr.coco.utils.Connection;
import it.cnr.coco.utils.ConnectionListener;

public class CoCo implements ConnectionListener {

    private static CoCo instance;
    private final Map<String, PropertyType> propertyTypes = new HashMap<>();
    private final Map<String, Type> types = new HashMap<>();
    private final Map<String, Item> items = new HashMap<>();
    private final Collection<CoCoListener> listeners = new HashSet<>();

    private CoCo() {
        registerPropertyType(new BoolPropertyType());
        registerPropertyType(new IntPropertyType());
        registerPropertyType(new FloatPropertyType());
        registerPropertyType(new StringPropertyType());
        registerPropertyType(new SymbolPropertyType());
        registerPropertyType(new ItemPropertyType());
        registerPropertyType(new JSONPropertyType());
    }

    public static CoCo getInstance() {
        if (instance == null)
            instance = new CoCo();
        return instance;
    }

    public void registerPropertyType(@NonNull PropertyType propertyType) {
        propertyTypes.put(propertyType.getName(), propertyType);
    }

    public Collection<Type> getTypes() {
        return types.values();
    }

    public Type getType(@NonNull String typeName) {
        return types.get(typeName);
    }

    public void setTypes(@NonNull JsonArray typeArray) {
        types.clear();
        for (JsonElement typeElement : typeArray) {
            JsonObject typeObject = typeElement.getAsJsonObject();
            types.put(typeObject.get("name").getAsString(),
                    new Type(typeObject.get("name").getAsString(), typeObject.get("data"), null, null));
        }
        for (JsonElement typeElement : typeArray) {
            JsonObject typeObject = typeElement.getAsJsonObject();
            Type type = Objects.requireNonNull(types.get(typeObject.get("name").getAsString()));
            refineType(type, typeObject);
        }
        for (Type type : types.values())
            for (CoCoListener listener : listeners)
                listener.new_type(type);
    }

    public Collection<Item> getItems() {
        return items.values();
    }

    public Item getItem(@NonNull String itemId) {
        return items.get(itemId);
    }

    public void setItems(@NonNull JsonArray itemArray) {
        items.clear();
        for (JsonElement itemElement : itemArray) {
            JsonObject itemObject = itemElement.getAsJsonObject();
            Set<Type> itemTypes = new HashSet<>();
            for (JsonElement typeElement : itemObject.getAsJsonArray("types")) {
                Type type = Objects.requireNonNull(types.get(typeElement.getAsString()));
                itemTypes.add(type);
            }
            Value value = itemObject.has("value") ? new Value(itemObject.get("value").getAsJsonObject().get("data"),
                    Instant.ofEpochMilli(itemObject.get("value").getAsJsonObject().get("timestamp").getAsLong()))
                    : null;
            Item item = new Item(itemObject.get("id").getAsString(), itemTypes, itemObject.get("data"), value);
            items.put(itemObject.get("id").getAsString(), item);
            for (CoCoListener listener : listeners)
                listener.new_item(item);
        }
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onReceivedMessage(@NonNull JsonObject message) {
        String msgType = (String) message.getAsJsonPrimitive(Connection.MSG_TYPE).getAsString();

        switch (Objects.requireNonNull(msgType)) {
            case "coco":
                if (message.has("types")) {
                    types.clear();
                    setTypes(message.getAsJsonObject("types").getAsJsonArray("array"));
                }
                if (message.has("items")) {
                    items.clear();
                    setItems(message.getAsJsonObject("items").getAsJsonArray("array"));
                }
                break;
            case "new_type":
                Type type = new Type(message.get("name").getAsString(), message.get("data"), null, null);
                types.put(message.get("name").getAsString(), type);
                refineType(type, message);
                for (CoCoListener listener : listeners)
                    listener.new_type(type);
                break;
            case "new_item":
                Set<Type> itemTypes = new HashSet<>();
                for (JsonElement typeElement : message.getAsJsonArray("types")) {
                    Type t = Objects.requireNonNull(types.get(typeElement.getAsString()));
                    itemTypes.add(t);
                }
                Value itemValue = message.has("value")
                        ? new Value(message.get("value").getAsJsonObject().get("data"),
                                Instant.ofEpochMilli(
                                        message.get("value").getAsJsonObject().get("timestamp").getAsLong()))
                        : null;
                Item item = new Item(message.get("id").getAsString(), itemTypes, message.get("data"), itemValue);
                items.put(message.get("id").getAsString(), item);
                for (CoCoListener listener : listeners)
                    listener.new_item(item);
                break;
        }
    }

    private void refineType(@NonNull Type type, @NonNull JsonObject type_message) {
        if (type_message.has("static_properties")) {
            Map<String, Property> static_properties = new HashMap<>();
            for (Map.Entry<String, JsonElement> entry : type_message.getAsJsonObject("static_properties").entrySet())
                static_properties.put(entry.getKey(),
                        Objects.requireNonNull(
                                propertyTypes.get(entry.getValue().getAsJsonObject().get("type").getAsString()))
                                .createProperty(this, entry.getValue()));
            type.static_properties = static_properties;
        }
        if (type_message.has("dynamic_properties")) {
            Map<String, Property> dynamic_properties = new HashMap<>();
            for (Map.Entry<String, JsonElement> entry : type_message.getAsJsonObject("dynamic_properties").entrySet())
                dynamic_properties.put(entry.getKey(),
                        Objects.requireNonNull(
                                propertyTypes.get(entry.getValue().getAsJsonObject().get("type").getAsString()))
                                .createProperty(this, entry.getValue()));
            type.dynamic_properties = dynamic_properties;
        }
    }

    @Override
    public void onConnectionFailed(@NonNull String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }

    public void addListener(@NonNull CoCoListener listener) {
        listeners.add(listener);
    }

    public void removeListener(@NonNull CoCoListener listener) {
        listeners.remove(listener);
    }
}
