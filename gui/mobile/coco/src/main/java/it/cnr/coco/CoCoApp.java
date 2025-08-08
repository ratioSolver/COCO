package it.cnr.coco;

import android.app.Application;

import it.cnr.coco.api.CoCo;
import it.cnr.coco.utils.Connection;
import it.cnr.coco.utils.Settings;

public class CoCoApp extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        // Load settings here
        Settings.getInstance().load(this);
        Connection.getInstance().addListener(CoCo.getInstance()); // Register CoCo as a listener for connection events
    }
}