package it.cnr.coco;

import static android.content.Context.MODE_PRIVATE;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import androidx.annotation.NonNull;
import android.content.Intent;
import it.cnr.coco.LogInActivity;

public class MainActivity extends Activity implements ConnectionListener {

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String token = getSharedPreferences("coco_prefs", MODE_PRIVATE).getString("token", null);
        if (token != null)
            Connection.getInstance().connect(token);
        else {
            Log.d(TAG, "No token found, redirecting to login");
            startActivity(new Intent(this, LogInActivity.class));
        }

        Connection.getInstance().addListener(this);
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onConnectionFailed(String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Connection.getInstance().removeListener(this);
    }
}
