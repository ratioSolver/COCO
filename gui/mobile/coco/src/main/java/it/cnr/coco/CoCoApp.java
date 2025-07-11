package it.cnr.coco;

import android.app.Application;

public class CoCoApp extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        // Load settings here
        Settings.getInstance().load(this);
    }
}