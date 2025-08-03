package it.cnr.coco;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Map;

public class CreateUserActivity extends Activity implements ConnectionListener {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_create_user); // Set the layout for this activity

        Connection.getInstance().addListener(this);
    }

    public void onCreateUserButtonClick(@NonNull View view) {
        EditText usernameEditText = findViewById(R.id.username);
        EditText passwordEditText = findViewById(R.id.password);

        Connection.getInstance().createUser(this, usernameEditText.getText().toString(),
                passwordEditText.getText().toString(), null); // Replace null with actual personal data if needed
    }

    @Override
    public void onConnectionEstablished() {
        // Redirect to MainActivity after successful user creation
        Intent intent = new Intent(this, MainActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        finish(); // Close the CreateUserActivity
    }

    @Override
    public void onReceivedMessage(@NonNull Map<String, Object> message) {
    }

    @Override
    public void onConnectionFailed(String errorMessage) {
        runOnUiThread(() -> Toast.makeText(this, "User creation failed: " + errorMessage, Toast.LENGTH_LONG).show());
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
