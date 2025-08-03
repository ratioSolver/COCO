package it.cnr.coco;

import androidx.annotation.NonNull;

import java.util.Map;

public interface ConnectionListener {
    /**
     * Called when the connection is established successfully.
     */
    void onConnectionEstablished();

    /**
     * Called when a message is received from the connection.
     *
     * @param message The message received, represented as a Map.
     */
    void onReceivedMessage(@NonNull Map<String, Object> message);

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
