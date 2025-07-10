package it.cnr.coco;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class SettingsActivity extends Activity {

    private EditText hostnameEditText;
    private EditText portEditText;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings); // Set the layout for this activity

        hostnameEditText = findViewById(R.id.hostname);
        portEditText = findViewById(R.id.port);

        // Load current settings into the EditTexts
        hostnameEditText.setText(Settings.getInstance().getHostname());
        portEditText.setText(String.valueOf(Settings.getInstance().getPort()));
    }

    public void onSaveButtonClick(@NonNull View view) {
        Settings.getInstance().setHostname(hostnameEditText.getText().toString());
        Settings.getInstance().setPort(Integer.parseInt(portEditText.getText().toString()));

        finish();
    }
}
