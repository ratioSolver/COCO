package it.cnr.coco;

import static android.content.Context.MODE_PRIVATE;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;

import com.google.firebase.messaging.FirebaseMessaging;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.reflect.TypeToken;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.HashSet;

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
    private static final String COCO_CONNECTION = "connection";
    private static final String TAG = "Connection";
    private static final String MSG_TYPE = "msg_type";
    private static final int RECONNECT_DELAY_MS = 5000; // 5 seconds
    private static final Gson gson = new Gson();
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final OkHttpClient client;
    private WebSocket socket;
    private String token;
    private Set<ConnectionListener> listeners = new HashSet<>();

    private Connection() {
        client = new OkHttpClient();
    }

    public static Connection getInstance() {
        if (instance == null)
            instance = new Connection();
        return instance;
    }

    public void login(@NonNull Context ctx, @NonNull String username, @NonNull String password) {
        Log.d(TAG, "Logging in with username: " + username);
        final Map<String, String> body = new HashMap<>();
        body.put("username", username);
        body.put("password", password);
        final Request.Builder builder = new Request.Builder()
                .url(Settings.getInstance().getHost() + "/login")
                .post(RequestBody.create(gson.toJson(body), MediaType.parse("application/json")));
        client.newCall(builder.build()).enqueue(new Callback() {
            @Override
            public void onFailure(@NonNull Call call, @NonNull IOException e) {
                Log.e(TAG, "Login failed", e);
                for (ConnectionListener listener : listeners)
                    listener.onConnectionFailed("Login failed: " + e.getMessage());
            }

            @Override
            public void onResponse(@NonNull Call call, @NonNull Response response) throws IOException {
                if (response.isSuccessful()) {
                    String responseBody = response.body().string();
                    Map<String, String> responseMap = gson.fromJson(responseBody, new TypeToken<Map<String, String>>() {
                    }.getType());
                    token = responseMap.get("token");
                    ctx.getSharedPreferences(COCO_CONNECTION, MODE_PRIVATE).edit().putString("token", token).apply();
                    connect(token);
                } else {
                    Log.e(TAG, "Login failed: " + response.message());
                    for (ConnectionListener listener : listeners)
                        listener.onConnectionFailed("Login failed: " + response.message());
                }
            }
        });
    }

    public void createUser(@NonNull Context ctx, @NonNull String username, @NonNull String password,
            JsonElement personal_data) {
        Log.d(TAG, "Creating user with username: " + username);
        final Map<String, Object> body = new HashMap<>();
        body.put("username", username);
        body.put("password", password);
        if (personal_data != null)
            body.put("personal_data", personal_data);
        final Request.Builder builder = new Request.Builder()
                .url(Settings.getInstance().getHost() + "/users")
                .post(RequestBody.create(gson.toJson(body), MediaType.parse("application/json")));
        if (token != null)
            builder.addHeader("Authorization", "Bearer " + token);
        client.newCall(builder.build()).enqueue(new Callback() {
            @Override
            public void onFailure(@NonNull Call call, @NonNull IOException e) {
                Log.e(TAG, "User creation failed", e);
                for (ConnectionListener listener : listeners)
                    listener.onConnectionFailed("User creation failed: " + e.getMessage());
            }

            @Override
            public void onResponse(@NonNull Call call, @NonNull Response response) throws IOException {
                if (response.isSuccessful()) {
                    String responseBody = response.body().string();
                    Map<String, String> responseMap = gson.fromJson(responseBody, new TypeToken<Map<String, String>>() {
                    }.getType());
                    token = responseMap.get("token");
                    ctx.getSharedPreferences(COCO_CONNECTION, MODE_PRIVATE).edit().putString("token", token).apply();
                    connect(token);
                } else {
                    Log.e(TAG, "User creation failed: " + response.message());
                    for (ConnectionListener listener : listeners)
                        listener.onConnectionFailed("User creation failed: " + response.message());
                }
            }
        });
    }

    public void connect(String token) {
        this.token = token;
        if (socket != null)
            socket.close(1000, "Reconnecting");
        Log.d(TAG, "Connecting to WebSocket server " + Settings.getInstance().getWsHost() + " with token: " + token);
        final Request request = new Request.Builder().url(Settings.getInstance().getWsHost()).build();
        socket = client.newWebSocket(request, this);
    }

    @Override
    public void onOpen(@NonNull WebSocket webSocket, @NonNull Response response) {
        Log.d(TAG, "WebSocket connection opened");
        FirebaseMessaging.getInstance().getToken().addOnCompleteListener(task -> {
            if (task.isSuccessful()) {
                Log.d(TAG, "FCM token retrieved successfully");
                String fcm = task.getResult();
                Map<String, String> body = new HashMap<>();
                body.put(MSG_TYPE, "login");
                body.put("token", token);
                body.put("fcm", fcm);
                webSocket.send(gson.toJson(body));
            } else {
                Log.e("Connection", "Failed to get FCM token", task.getException());
                for (ConnectionListener listener : listeners)
                    listener.onConnectionFailed("Failed to get FCM token");
            }
        });
    }

    @Override
    public void onMessage(@NonNull WebSocket webSocket, @NonNull String text) {
        Log.d(TAG, "WebSocket message received: " + text);
        Map<String, Object> message = gson.fromJson(text, new TypeToken<Map<String, Object>>() {
        }.getType());
        String msgType = (String) message.get(MSG_TYPE);

        if ("login".equals(msgType)) {
            Log.d(TAG, "Login successful");
            for (ConnectionListener listener : listeners)
                listener.onConnectionEstablished();
        }
    }

    @Override
    public void onFailure(@NonNull WebSocket webSocket, @NonNull Throwable t, Response response) {
        Log.e(TAG, "WebSocket connection failed", t);
        for (ConnectionListener listener : listeners)
            listener.onConnectionFailed("WebSocket connection failed: " + t.getMessage());
        handler.postDelayed(() -> connect(token), RECONNECT_DELAY_MS);
    }

    @Override
    public void onClosed(@NonNull WebSocket webSocket, int code, @NonNull String reason) {
        Log.d(TAG, "WebSocket connection closed: " + reason);
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