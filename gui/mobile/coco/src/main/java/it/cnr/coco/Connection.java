package it.cnr.coco;

import it.cnr.coco.Settings;
import it.cnr.coco.api.CoCoService;
import okhttp3.OkHttpClient;
import okhttp3.WebSocketListener;
import okhttp3.WebSocket;
import okhttp3.Request;
import okhttp3.Response;
import retrofit2.Retrofit;

public class Connection extends WebSocketListener {

    private static Connection instance;
    private final CoCoService service;
    private final OkHttpClient client;

    private Connection() {
        client = new OkHttpClient();
        client.newWebSocket(new Request.Builder().url(Settings.getInstance().getWsHost()).build(), this);
        Retrofit retrofit = new Retrofit.Builder().baseUrl(Settings.getInstance().getHost()).client(client).build();
        service = retrofit.create(CoCoService.class);
    }

    public static Connection getInstance() {
        if (instance == null)
            instance = new Connection();
        return instance;
    }

    @Override
    public void onOpen(WebSocket webSocket, Response response) {
    }

    @Override
    public void onMessage(WebSocket webSocket, String text) {
    }

    @Override
    public void onFailure(WebSocket webSocket, Throwable t, Response response) {
    }
}