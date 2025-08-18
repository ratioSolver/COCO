package it.cnr.coco;

import android.Manifest;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.google.gson.JsonObject;

import it.cnr.coco.api.CoCo;
import it.cnr.coco.api.CoCoListener;
import it.cnr.coco.api.Item;
import it.cnr.coco.api.Type;

import it.cnr.coco.utils.Camera;
import it.cnr.coco.utils.Connection;
import it.cnr.coco.utils.ConnectionListener;
import it.cnr.coco.utils.Face;
import it.cnr.coco.utils.Language;
import it.cnr.coco.utils.Settings;

import java.util.ArrayList;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements CoCoListener, ConnectionListener {

    private static final String TAG = "MainActivity";
    private static final int REQUEST_RECORD_AUDIO_PERMISSION = 1;
    private static final int REQUEST_CAMERA_PERMISSION = 2;
    private Language language; // Instance of the Language class to handle speech and text-to-speech
    private ImageView robotFace; // ImageView for the robot's face
    private Face face; // Instance of the Face class to handle robot's face
    private Camera camera; // Instance of the Camera class to handle camera operations

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main); // Set the layout for this activity

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        camera = new Camera(); // Initialize the Camera class

        robotFace = findViewById(R.id.robot_face);

        Connection.getInstance().addListener(this); // Register this activity as a listener for connection events

        // Request microphone permission
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED)
            ActivityCompat.requestPermissions(this, new String[] { Manifest.permission.RECORD_AUDIO },
                    REQUEST_RECORD_AUDIO_PERMISSION);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED)
            ActivityCompat.requestPermissions(this, new String[] { Manifest.permission.CAMERA },
                    REQUEST_CAMERA_PERMISSION);
        else
            camera.startCamera(this); // Start the camera if permission is granted

        if (!Connection.getInstance().isConnected())
            if (Settings.getInstance().hasUsers()) {
                String token = getSharedPreferences(Connection.COCO_CONNECTION, MODE_PRIVATE).getString("token", null);
                if (token != null) // Users with a valid token can connect
                    Connection.getInstance().connect(token);
                else {
                    Log.d(TAG, "No token found, redirecting to login");
                    Intent intent = new Intent(this, LogInActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                    finish(); // Close this activity
                }
            } else
                Connection.getInstance().connect(null);

        CoCo.getInstance().addListener(this); // Register this activity as a listener for CoCo events
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        if (face != null && findViewById(R.id.robot_face) != null) // Set the face view for the robot
            face.setFaceView(findViewById(R.id.robot_face));
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
    public void new_type(Type type) {
    }

    @Override
    public void new_item(Item item) {
        if (item.getType().getName().equals("Robot")) {
            Log.d(TAG, "New robot item received: " + item.getId());
            runOnUiThread(() -> {
                // Initialize the Language class to handle speech and text-to-speech
                language = new Language(this, item);
                // Initialize the Face class to handle robot's face
                face = new Face(this, item, robotFace);
            });
        }
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onReceivedMessage(@NonNull JsonObject message) {
    }

    @Override
    public void onConnectionFailed(@NonNull String errorMessage) {
        runOnUiThread(() -> Toast.makeText(this, "Connection failed: " + errorMessage, Toast.LENGTH_LONG).show());
        if (Settings.getInstance().hasUsers()) {
            Intent intent = new Intent(this, LogInActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
            finish(); // Close this activity
        } else {
            Intent intent = new Intent(this, SettingsActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
        }
    }

    @Override
    public void onConnectionClosed() {
        if (Settings.getInstance().hasUsers()) {
            Intent intent = new Intent(this, LogInActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
            finish(); // Close this activity
        } else {
            Intent intent = new Intent(this, SettingsActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
            @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_RECORD_AUDIO_PERMISSION)
            if (grantResults.length > 0 && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Microphone permission is required!", Toast.LENGTH_SHORT).show();
                finish();
            } else
                Log.d(TAG, "Microphone permission granted");
        else if (requestCode == REQUEST_CAMERA_PERMISSION)
            if (grantResults.length > 0 && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Camera permission is required!", Toast.LENGTH_SHORT).show();
                finish();
            } else {
                Log.d(TAG, "Camera permission granted");
                camera.startCamera(this); // Start the camera if permission is granted
            }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (language != null)
            language.destroy(); // Clean up the Language instance
        if (face != null)
            face.destroy(); // Clean up the Face instance
        camera.destroy(); // Clean up the Camera instance
        // Unregister this activity as a listener for connection events
        Connection.getInstance().removeListener(this);
        // Unregister this activity as a listener for CoCo events
        CoCo.getInstance().removeListener(this);
    }
}
