package it.cnr.coco;

import java.util.Map;
import java.util.Objects;

public class Settings {

    private static Settings instance;

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
        this.secure = secure;
    }

    public String getHostname() {
        return hostname;
    }

    public void setHostname(String hostname) {
        this.hostname = hostname;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public String getWsPath() {
        return ws_path;
    }

    public void setWsPath(String ws_path) {
        this.ws_path = ws_path;
    }

    public String getHost() {
        return (secure ? "https://" : "http://") + hostname + ":" + port;
    }

    public String getWsHost() {
        return (secure ? "wss://" : "ws://") + hostname + ":" + port + ws_path;
    }

    public void load(Map<String, String> settings) {
        if (settings.containsKey("secure"))
            this.secure = Boolean.parseBoolean(settings.get("secure"));
        if (settings.containsKey("hostname"))
            this.hostname = settings.get("hostname");
        if (settings.containsKey("port"))
            this.port = Integer.parseInt(Objects.requireNonNull(settings.get("port")));
        if (settings.containsKey("ws_path"))
            this.ws_path = settings.get("ws_path");
    }
}
