package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonObject;

import java.util.HashMap;
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

    public Type getType(@NonNull String typeName) {
        return types.get(typeName);
    }

    public Item getItem(@NonNull String itemId) {
        return items.get(itemId);
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onReceivedMessage(@NonNull JsonObject message) {
        String msgType = (String) message.getAsJsonPrimitive(Connection.MSG_TYPE).getAsString();

        switch (Objects.requireNonNull(msgType)) {
            case "coco":
                break;
            case "new_type":
                break;
            case "new_item":
                break;
        }
    }

    @Override
    public void onConnectionFailed(String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }
}
