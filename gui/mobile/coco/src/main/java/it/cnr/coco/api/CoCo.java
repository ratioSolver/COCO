package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;

import it.cnr.coco.utils.Connection;
import it.cnr.coco.utils.ConnectionListener;

public class CoCo implements ConnectionListener {

    private static CoCo instance;
    private final Map<String, PropertyType> propertyTypes = new HashMap<>();
    private final Map<String, Type> types = new HashMap<>();
    private final Map<String, Item> items = new HashMap<>();

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
                    new Type(typeObject.get("name").getAsString(), null, typeObject.get("data"), null, null));
        }
        for (JsonElement typeElement : typeArray) {
            JsonObject typeObject = typeElement.getAsJsonObject();
            refineType(Objects.requireNonNull(types.get(typeObject.get("name").getAsString())),
                    typeObject);
        }
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
            items.put(itemObject.get("id").getAsString(),
                    new Item(itemObject.get("id").getAsString(),
                            Objects.requireNonNull(types.get(itemObject.get("type").getAsString())),
                            itemObject.get("data")));
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
                    for (Map.Entry<String, JsonElement> entry : message.getAsJsonObject("types").entrySet())
                        types.put(entry.getKey(),
                                new Type(entry.getKey(), null, entry.getValue().getAsJsonObject().get("data"), null,
                                        null));
                    for (Map.Entry<String, JsonElement> entry : message.getAsJsonObject("types").entrySet())
                        refineType(Objects.requireNonNull(types.get(entry.getKey())),
                                entry.getValue().getAsJsonObject());
                }
                if (message.has("items")) {
                    items.clear();
                    for (Map.Entry<String, JsonElement> entry : message.getAsJsonObject("items").entrySet())
                        items.put(entry.getKey(),
                                new Item(entry.getKey(),
                                        Objects.requireNonNull(types
                                                .get(entry.getValue().getAsJsonObject().get("type").getAsString())),
                                        entry.getValue().getAsJsonObject().get("data")));
                }
                break;
            case "new_type":
                break;
            case "new_item":
                break;
        }
    }

    private void refineType(@NonNull Type type, @NonNull JsonObject type_message) {
        if (type_message.has("parents")) {
            Collection<Type> parents = new HashSet<>();
            for (JsonElement parent : type_message.getAsJsonArray("parents"))
                parents.add(Objects.requireNonNull(types.get(parent.getAsString())));
            type.setParents(parents);
        }
        if (type_message.has("static_properties")) {
            Map<String, Property> staticProperties = new HashMap<>();
            for (Map.Entry<String, JsonElement> entry : type_message.getAsJsonObject("static_properties").entrySet())
                staticProperties.put(entry.getKey(),
                        Objects.requireNonNull(
                                propertyTypes.get(entry.getValue().getAsJsonObject().get("type").getAsString()))
                                .createProperty(this, entry.getValue()));
            type.setStaticProperties(staticProperties);
        }
        if (type_message.has("dynamic_properties")) {
            Map<String, Property> dynamicProperties = new HashMap<>();
            for (Map.Entry<String, JsonElement> entry : type_message.getAsJsonObject("dynamic_properties").entrySet())
                dynamicProperties.put(entry.getKey(),
                        Objects.requireNonNull(
                                propertyTypes.get(entry.getValue().getAsJsonObject().get("type").getAsString()))
                                .createProperty(this, entry.getValue()));
            type.setDynamicProperties(dynamicProperties);
        }
    }

    @Override
    public void onConnectionFailed(@NonNull String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }
}
