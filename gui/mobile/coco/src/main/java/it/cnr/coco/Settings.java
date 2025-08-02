package it.cnr.coco;

import static android.content.Context.MODE_PRIVATE;

import java.util.Map;
import java.util.Objects;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import androidx.annotation.NonNull;

public class Settings {

    private static final String COCO_SETTINGS = "coco_settings";
    private static final String TAG = "Settings";
    private static Settings instance;

    private Context ctx;
    private boolean has_users = false;
    private boolean secure = true;
    private String hostname = "coco.cnr.it";
    private int port = 443;
    private String ws_path = "/coco";

    private Settings() {
    }

    public static Settings getInstance() {
        if (instance == null)
            instance = new Settings();
        return instance;
    }

    public boolean isSecure() {
        return secure;
    }

    public void setSecure(boolean secure) {
        Log.d(TAG, "Setting secure: " + secure);
        this.secure = secure;
        ctx.getSharedPreferences(COCO_SETTINGS, MODE_PRIVATE).edit().putBoolean("secure", secure).apply();
    }

    public boolean hasUsers() {
        return has_users;
    }

    public void setHasUsers(boolean has_users) {
        Log.d(TAG, "Setting has_users: " + has_users);
        this.has_users = has_users;
        ctx.getSharedPreferences(COCO_SETTINGS, MODE_PRIVATE).edit().putBoolean("has_users", has_users).apply();
    }

    public String getHostname() {
        return hostname;
    }

    public void setHostname(String hostname) {
        Log.d(TAG, "Setting hostname: " + hostname);
        this.hostname = hostname;
        ctx.getSharedPreferences(COCO_SETTINGS, MODE_PRIVATE).edit().putString("hostname", hostname).apply();
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        Log.d(TAG, "Setting port: " + port);
        this.port = port;
        ctx.getSharedPreferences(COCO_SETTINGS, MODE_PRIVATE).edit().putInt("port", port).apply();
    }

    public String getWsPath() {
        return ws_path;
    }

    public void setWsPath(String ws_path) {
        Log.d(TAG, "Setting WebSocket path: " + ws_path);
        this.ws_path = ws_path;
        ctx.getSharedPreferences(COCO_SETTINGS, MODE_PRIVATE).edit().putString("ws_path", ws_path).apply();
    }

    public String getHost() {
        return (secure ? "https://" : "http://") + hostname + ":" + port;
    }

    public String getWsHost() {
        return (secure ? "wss://" : "ws://") + hostname + ":" + port + ws_path;
    }

    public void load(@NonNull Context ctx) {
        this.ctx = ctx;
        SharedPreferences prefs = ctx.getSharedPreferences(COCO_SETTINGS, MODE_PRIVATE);
        if (prefs.contains("has_users"))
            this.has_users = prefs.getBoolean("has_users", false);
        if (prefs.contains("secure"))
            this.secure = prefs.getBoolean("secure", true);
        if (prefs.contains("hostname"))
            this.hostname = prefs.getString("hostname", "coco.cnr.it");
        if (prefs.contains("port"))
            this.port = prefs.getInt("port", 443);
        if (prefs.contains("ws_path"))
            this.ws_path = prefs.getString("ws_path", "/coco");
    }
}
