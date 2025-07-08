package it.cnr.coco;

import it.cnr.coco.CoCoService;
import it.cnr.coco.Settings;
import retrofit2.Retrofit;

public class Connection {

    private static Connection instance;
    private final CoCoService service;

    private Connection() {
        Retrofit retrofit = new Retrofit.Builder().baseUrl(Settings.getInstance().getServerUrl()).build();
        service = retrofit.create(CoCoService.class);
    }

    public static Connection getInstance() {
        if (instance == null)
            instance = new Connection();
        return instance;
    }
}