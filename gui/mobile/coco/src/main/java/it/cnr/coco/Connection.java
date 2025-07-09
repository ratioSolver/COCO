package it.cnr.coco;

import android.os.Handler;
import android.os.Looper;
import it.cnr.coco.Settings;
import java.util.HashMap;
import java.util.Map;
import android.content.Context;
import com.google.gson.Gson;
import okhttp3.OkHttpClient;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okhttp3.WebSocket;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.Callback;
import okhttp3.Call;
import okhttp3.RequestBody;
import okhttp3.MediaType;
import android.util.Log;
import com.google.firebase.messaging.FirebaseMessaging;
import com.google.gson.JsonElement;
import com.google.gson.reflect.TypeToken;
import static android.content.Context.MODE_PRIVATE;
import java.io.IOException;

public class Connection extends WebSocketListener {

    private static Connection instance;
    private static final String TAG = "Connection";
    private static final String MSG_TYPE = "msg_type";
    private static final int RECONNECT_DELAY_MS = 5000; // 5 seconds
    private static final Gson gson = new Gson();
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final OkHttpClient client;
    private WebSocket socket;
    private String token;

    private Connection() {
        client = new OkHttpClient();
    }

    public static Connection getInstance() {
        if (instance == null)
            instance = new Connection();
        return instance;
    }

    public void login(Context ctx, String username, String password) {
        Log.d(TAG, "Logging in with username: " + username);
        final Map<String, String> body = new HashMap<>();
        body.put("username", username);
        body.put("password", password);
        final Request.Builder builder = new Request.Builder()
                .url(Settings.getInstance().getHost() + "/login")
                .post(RequestBody.create(MediaType.parse("application/json"), gson.toJson(body)));
        client.newCall(builder.build()).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                Log.e(TAG, "Login failed", e);
                e.printStackTrace();
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                if (response.isSuccessful()) {
                    String responseBody = response.body().string();
                    Map<String, String> responseMap = gson.fromJson(responseBody, new TypeToken<Map<String, String>>() {
                    }.getType());
                    token = responseMap.get("token");
                    ctx.getSharedPreferences("app_prefs", MODE_PRIVATE).edit().putString("token", token).apply();
                    connect(token);
                } else {
                    Log.e(TAG, "Login failed: " + response.message());
                }
            }
        });
    }

    public void createUser(Context ctx, String username, String password, JsonElement personal_data) {
        Log.d(TAG, "Creating user with username: " + username);
        final Map<String, Object> body = new HashMap<>();
        body.put("username", username);
        body.put("password", password);
        if (personal_data != null)
            body.put("personal_data", personal_data);
        final Request.Builder builder = new Request.Builder()
                .url(Settings.getInstance().getHost() + "/users")
                .post(RequestBody.create(MediaType.parse("application/json"), gson.toJson(body)));
        if (token != null)
            builder.addHeader("Authorization", "Bearer " + token);
        client.newCall(builder.build()).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                Log.e(TAG, "User creation failed", e);
                e.printStackTrace();
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                if (response.isSuccessful()) {
                    String responseBody = response.body().string();
                    Map<String, String> responseMap = gson.fromJson(responseBody, new TypeToken<Map<String, String>>() {
                    }.getType());
                    token = responseMap.get("token");
                    ctx.getSharedPreferences("app_prefs", MODE_PRIVATE).edit().putString("token", token).apply();
                    connect(token);
                } else {
                    Log.e(TAG, "User creation failed: " + response.message());
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
    public void onOpen(WebSocket webSocket, Response response) {
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
            }
        });
    }

    @Override
    public void onMessage(WebSocket webSocket, String text) {
    }

    @Override
    public void onFailure(WebSocket webSocket, Throwable t, Response response) {
        Log.e(TAG, "WebSocket connection failed", t);
        handler.postDelayed(() -> connect(token), RECONNECT_DELAY_MS);
    }

    @Override
    public void onClosed(WebSocket webSocket, int code, String reason) {
        Log.d(TAG, "WebSocket connection closed: " + reason);
    }
}