package it.cnr.coco;

public interface ConnectionListener {
    /**
     * Called when the connection is established successfully.
     */
    void onConnectionEstablished();

    /**
     * Called when the connection fails.
     *
     * @param errorMessage The error message describing the failure.
     */
    void onConnectionFailed(String errorMessage);

    /**
     * Called when the connection is closed.
     */
    void onConnectionClosed();
}
