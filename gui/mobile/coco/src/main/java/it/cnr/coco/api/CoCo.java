package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.HashMap;
import java.util.Map;

import it.cnr.coco.Connection;
import it.cnr.coco.ConnectionListener;

public class CoCo implements ConnectionListener {

    private static CoCo instance;
    private final Map<String, Type> types = new HashMap<>();

    private CoCo() {
    }

    public static CoCo getInstance() {
        if (instance == null)
            instance = new CoCo();
        return instance;
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onReceivedMessage(@NonNull Map<String, Object> message) {
        String msgType = (String) message.get(Connection.MSG_TYPE);

        switch (msgType) {
            case "coco":
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
