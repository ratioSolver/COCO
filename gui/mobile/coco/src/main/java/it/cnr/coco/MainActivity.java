package it.cnr.coco;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity implements ConnectionListener {

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main); // Set the layout for this activity

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        String token = getSharedPreferences("coco_prefs", MODE_PRIVATE).getString("token", null);
        if (token != null)
            Connection.getInstance().connect(token);
        else {
            Log.d(TAG, "No token found, redirecting to login");
            startActivity(new Intent(this, LogInActivity.class));
        }

        Settings.getInstance().load(this); // Load settings from shared preferences

        Connection.getInstance().addListener(this); // Register this activity as a listener for connection events
    }

    @Override
    public boolean onCreateOptionsMenu(@NonNull Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true; // Return true to display the menu
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        if (item.getItemId() == R.id.action_settings) {
            Intent intent = new Intent(this, SettingsActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
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
