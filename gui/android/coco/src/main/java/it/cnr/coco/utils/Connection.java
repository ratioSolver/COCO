package it.cnr.coco.utils;

import static android.content.Context.MODE_PRIVATE;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;

import com.google.firebase.messaging.FirebaseMessaging;
import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import java.io.IOException;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.Executors;

import it.cnr.coco.api.CoCo;
import it.cnr.coco.api.Item;
import it.cnr.coco.api.Type;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;

public class Connection extends WebSocketListener {

    private static Connection instance;
    public static final String COCO_CONNECTION = "coco_connection";
    private static final String TAG = "Connection";
    public static final String MSG_TYPE = "msg_type";
    private static final int RECONNECT_DELAY_MS = 60000; // 1 minute
    private static final Gson gson = new Gson();
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final OkHttpClient client;
    private WebSocket socket;
    private String token;
    private boolean connected = false;
    private final Collection<ConnectionListener> listeners = new HashSet<>();

    private Connection() {
        client = new OkHttpClient();
    }

    public static Connection getInstance() {
        if (instance == null)
            instance = new Connection();
        return instance;
    }

    /**
     * Returns whether the connection is currently established.
     *
     * @return {@code true} if connected, {@code false} otherwise
     */
    public boolean isConnected() {
        return connected;
    }

    /**
     * Returns the authentication token associated with this connection.
     *
     * @return the current authentication token as a {@code String}
     */
    public String getToken() {
        return token;
    }

    /**
     * Attempts to log in a user with the provided username and password.
     * Sends a POST request to the server's /login endpoint with the credentials as
     * JSON.
     * On successful login, retrieves the authentication token from the response,
     * saves it in the shared preferences, and initiates a connection using the
     * token.
     * Notifies registered {@link ConnectionListener}s of success or failure.
     *
     * @param ctx      the Android context, used for accessing shared preferences
     * @param username the username to log in with (must not be null)
     * @param password the password to log in with (must not be null)
     */
    public void login(@NonNull Context ctx, @NonNull String username, @NonNull String password) {
        Log.d(TAG, "Logging in with username: " + username);
        final Map<String, String> body = new HashMap<>();
        body.put("username", username);
        body.put("password", password);
        final Request.Builder builder = new Request.Builder().url(Settings.getInstance().getHost() + "/login")
                .post(RequestBody.create(gson.toJson(body), MediaType.parse("application/json")));
        try (Response response = client.newCall(builder.build()).execute()) {
            if (response.isSuccessful()) {
                JsonObject responseBody = gson.fromJson(response.body().charStream(), JsonObject.class);
                token = responseBody.getAsJsonPrimitive("token").getAsString();
                ctx.getSharedPreferences(COCO_CONNECTION, MODE_PRIVATE).edit().putString("token", token).apply();
                connect(token);
            } else {
                Log.e(TAG, "Login failed: " + response.message());
                for (ConnectionListener listener : listeners)
                    listener.onConnectionFailed("Login failed: " + response.message());
            }
        } catch (IOException e) {
            Log.e(TAG, "Error during login", e);
            for (ConnectionListener listener : listeners)
                listener.onConnectionFailed("Login failed: " + e.getMessage());
        }
    }

    /**
     * Creates a new user with the specified username and password.
     * Sends a POST request to the server's /users endpoint with the user data as
     * JSON.
     * If the user is created successfully and {@code connect} is true, it connects
     * using the new token.
     *
     * @param ctx           the Android context, used for accessing shared
     *                      preferences
     * @param username      the username for the new user (must not be null)
     * @param password      the password for the new user (must not be null)
     * @param personal_data additional personal data as a JsonElement (can be null)
     * @param connect       whether to connect immediately after creating the user
     */
    public void createUser(@NonNull Context ctx, @NonNull String username, @NonNull String password,
            JsonElement personal_data, boolean connect) {
        Log.d(TAG, "Creating user with username: " + username);
        final Map<String, Object> body = new HashMap<>();
        body.put("username", username);
        body.put("password", password);
        if (personal_data != null)
            body.put("personal_data", personal_data);
        final Request.Builder builder = new Request.Builder().url(Settings.getInstance().getHost() + "/users")
                .post(RequestBody.create(gson.toJson(body), MediaType.parse("application/json")));
        if (token != null)
            builder.addHeader("Authorization", "Bearer " + token);
        try (Response response = client.newCall(builder.build()).execute()) {
            if (response.isSuccessful()) {
                Log.d(TAG, "User created successfully");
                if (connect) {
                    JsonObject responseBody = gson.fromJson(response.body().charStream(), JsonObject.class);
                    token = responseBody.getAsJsonPrimitive("token").getAsString();
                    ctx.getSharedPreferences(COCO_CONNECTION, MODE_PRIVATE).edit().putString("token", token).apply();
                    connect(token);
                }
            } else
                Log.e(TAG, "User creation failed: " + response.message());
        } catch (IOException e) {
            Log.e(TAG, "Error during user creation", e);
        }
    }

    /**
     * Fetches types from the server.
     * Constructs a GET request to the /types endpoint and retrieves the types as a
     * JsonArray.
     * If the request is successful, updates the CoCo instance with the fetched
     * types.
     *
     * @param token the authentication token (can be null)
     * @return a collection of fetched types, or an empty collection if the request
     *         fails
     */
    @NonNull
    public Collection<Type> getTypes(String token) {
        Log.d(TAG, "Fetching types from server");
        final Request.Builder builder = new Request.Builder().url(Settings.getInstance().getHost() + "/types").get();
        if (token != null)
            builder.addHeader("Authorization", "Bearer " + token);
        try (Response response = client.newCall(builder.build()).execute()) {
            if (response.isSuccessful()) {
                CoCo.getInstance().setTypes(gson.fromJson(response.body().charStream(), JsonArray.class));
                return CoCo.getInstance().getTypes();
            } else
                Log.e(TAG, "Failed to fetch types: " + response.message());
        } catch (IOException e) {
            Log.e(TAG, "Error fetching types", e);
        }
        return new HashSet<>();
    }

    /**
     * Fetches items from the server with optional filters.
     * Constructs a URL with the specified filters and sends a GET request to the
     * /items endpoint.
     * If the request is successful, updates the CoCo instance with the fetched
     * items.
     *
     * @param filters a map of filters to apply (can be empty)
     * @param token   the authentication token (can be null)
     * @return a collection of fetched items, or an empty collection if the request
     *         fails
     */
    @NonNull
    public Collection<Item> getItems(Map<String, String> filters, String token) {
        Log.d(TAG, "Fetching items from server" + (filters.isEmpty() ? "" : " with filters: " + filters));
        StringBuilder urlBuilder = new StringBuilder(Settings.getInstance().getHost() + "/items");
        if (!filters.isEmpty()) {
            urlBuilder.append("?");
            for (Map.Entry<String, String> entry : filters.entrySet())
                urlBuilder.append(entry.getKey()).append("=").append(entry.getValue()).append("&");
            urlBuilder.setLength(urlBuilder.length() - 1); // Remove trailing '&'
        }
        final Request.Builder builder = new Request.Builder().url(urlBuilder.toString()).get();
        if (token != null)
            builder.addHeader("Authorization", "Bearer " + token);
        try (Response response = client.newCall(builder.build()).execute()) {
            if (response.isSuccessful()) {
                CoCo.getInstance().setItems(gson.fromJson(response.body().charStream(), JsonArray.class));
                return CoCo.getInstance().getItems();
            } else
                Log.e(TAG, "Failed to fetch items: " + response.message());
        } catch (IOException e) {
            Log.e(TAG, "Error fetching items", e);
        }
        return new HashSet<>();
    }

    /**
     * Adds a new FCM token for the user with the specified ID.
     * Sends a POST request to the server's /fcm_tokens endpoint with the user ID
     * and
     * token as JSON.
     *
     * @param id    the user ID (must not be null)
     * @param token the FCM token to add (must not be null)
     */
    public void newToken(@NonNull String id, @NonNull String token) {
        Log.d(TAG, "Adding new token for user ID: " + id);
        final Map<String, String> body = new HashMap<>();
        body.put("id", id);
        body.put("token", token);
        final Request.Builder builder = new Request.Builder().url(Settings.getInstance().getHost() + "/fcm_tokens")
                .post(RequestBody.create(gson.toJson(body), MediaType.parse("application/json")));
        if (this.token != null)
            builder.addHeader("Authorization", "Bearer " + this.token);
        try (Response response = client.newCall(builder.build()).execute()) {
            if (response.isSuccessful())
                Log.d(TAG, "New token added successfully");
            else
                Log.e(TAG, "Failed to add new token: " + response.message());
        } catch (IOException e) {
            Log.e(TAG, "Error adding new token", e);
        }
    }

    /**
     * Connects to the WebSocket server using the provided token.
     * If the token is null, it connects without authentication.
     * Notifies registered {@link ConnectionListener}s of connection success or
     * failure.
     *
     * @param token the authentication token (can be null)
     */
    public void connect(String token) {
        this.token = token;
        if (socket != null)
            socket.close(1000, "Reconnecting");
        Log.d(TAG, "Connecting to WebSocket server " + Settings.getInstance().getWsHost()
                + (token != null ? " with token " + token : ""));
        final Request request = new Request.Builder().url(Settings.getInstance().getWsHost()).build();
        socket = client.newWebSocket(request, this);
    }

    /**
     * Publishes a message to the server using the WebSocket connection.
     * Sends a POST request to the /login endpoint with the provided item and
     * message as JSON.
     *
     * @param item    the item associated with the message (must not be null)
     * @param message the message to publish as a JsonObject (must not be null)
     */
    public void publish(@NonNull Item item, @NonNull JsonObject message) {
        final Request.Builder builder = new Request.Builder()
                .url(Settings.getInstance().getHost() + "/data/" + item.getId())
                .post(RequestBody.create(gson.toJson(message), MediaType.parse("application/json")));
        if (token != null)
            builder.addHeader("Authorization", "Bearer " + token);
        try (Response response = client.newCall(builder.build()).execute()) {
            if (!response.isSuccessful())
                Log.d(TAG, "Message published successfully");
            else
                Log.e(TAG, "Failed to publish message: " + response.message());
        } catch (IOException e) {
            Log.e(TAG, "Error publishing message", e);
            for (ConnectionListener listener : listeners)
                listener.onConnectionFailed("Failed to publish message: " + e.getMessage());
        }
    }

    public void disconnect() {
        if (socket != null) {
            socket.close(1000, "User requested disconnect");
            socket = null;
        }
    }

    @Override
    public void onOpen(@NonNull WebSocket webSocket, @NonNull Response response) {
        Log.d(TAG, "WebSocket connection opened");
        if (Settings.getInstance().hasUsers()) { // If there are users, send the login message
            Map<String, String> body = new HashMap<>();
            body.put(MSG_TYPE, "login");
            body.put("token", token);
            webSocket.send(gson.toJson(body));
            FirebaseMessaging.getInstance().getToken().addOnCompleteListener(task -> {
                if (task.isSuccessful()) {
                    Log.d(TAG, "FCM token retrieved successfully");
                    String fcm_token = task.getResult();
                    Log.d(TAG, "FCM Token: " + fcm_token);
                    Executors.newSingleThreadExecutor().execute(() -> newToken(token, fcm_token));
                } else
                    Log.e("Connection", "Failed to get FCM token", task.getException());
            });
        } else { // No users, just notify listeners
            connected = true;
            for (ConnectionListener listener : listeners)
                listener.onConnectionEstablished();
        }
    }

    @Override
    public void onMessage(@NonNull WebSocket webSocket, @NonNull String text) {
        Log.d(TAG, "WebSocket message received: " + text);
        JsonObject message = gson.fromJson(text, JsonObject.class);
        String msgType = message.getAsJsonPrimitive(MSG_TYPE).getAsString();

        switch (msgType) {
            case "login":
                Log.d(TAG, "Login successful");
                connected = true;
                for (ConnectionListener listener : listeners)
                    listener.onConnectionEstablished();
                break;
            default:
                for (ConnectionListener listener : listeners)
                    listener.onReceivedMessage(message);
        }
    }

    @Override
    public void onFailure(@NonNull WebSocket webSocket, @NonNull Throwable t, Response response) {
        Log.e(TAG, "WebSocket connection failed", t);
        connected = false;
        for (ConnectionListener listener : listeners)
            listener.onConnectionFailed("WebSocket connection failed: " + t.getMessage());
        handler.postDelayed(() -> connect(token), RECONNECT_DELAY_MS);
    }

    @Override
    public void onClosed(@NonNull WebSocket webSocket, int code, @NonNull String reason) {
        Log.d(TAG, "WebSocket connection closed: " + reason);
        connected = false;
        for (ConnectionListener listener : listeners)
            listener.onConnectionClosed();
    }

    public void addListener(@NonNull ConnectionListener listener) {
        listeners.add(listener);
    }

    public void removeListener(@NonNull ConnectionListener listener) {
        listeners.remove(listener);
    }
}