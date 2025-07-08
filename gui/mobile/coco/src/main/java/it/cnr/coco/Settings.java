package it.cnr.coco;

public class Settings {

    private static Settings instance;

    private String serverUrl = "https://coco.cnr.it";

    private Settings() {
    }

    public static Settings getInstance() {
        if (instance == null)
            instance = new Settings();
        return instance;
    }

    public String getServerUrl() {
        return serverUrl;
    }

    public void setServerUrl(String serverUrl) {
        this.serverUrl = serverUrl;
    }
}
