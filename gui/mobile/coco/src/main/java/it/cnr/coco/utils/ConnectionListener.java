package it.cnr.coco.utils;

import androidx.annotation.NonNull;

import com.google.gson.JsonObject;

public interface ConnectionListener {
    /**
     * Called when the connection is established successfully.
     */
    void onConnectionEstablished();

    /**
     * Called when a message is received from the server.
     *
     * @param message The received message as a JSON object.
     */
    void onReceivedMessage(@NonNull JsonObject message);

    /**
     * Called when the connection fails.
     *
     * @param errorMessage The error message describing the failure.
     */
    void onConnectionFailed(@NonNull String errorMessage);

    /**
     * Called when the connection is closed.
     */
    void onConnectionClosed();
}
