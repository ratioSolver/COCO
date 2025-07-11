package it.cnr.coco;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class LogInActivity extends Activity implements ConnectionListener {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login); // Set the layout for this activity

        Connection.getInstance().addListener(this);
    }

    public void onLoginButtonClick(@NonNull View view) {
        EditText usernameEditText = findViewById(R.id.username);
        EditText passwordEditText = findViewById(R.id.password);

        Connection.getInstance().login(this, usernameEditText.getText().toString(),
                passwordEditText.getText().toString());
    }

    public void onSignUpClicked(@NonNull View view) {
        // Redirect to CreateUserActivity for user registration
        Intent intent = new Intent(this, CreateUserActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        finish(); // Close the LogInActivity
    }

    @Override
    public void onConnectionEstablished() {
        // Redirect to MainActivity after successful login
        Intent intent = new Intent(this, MainActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        finish(); // Close the LogInActivity
    }

    @Override
    public void onConnectionFailed(String errorMessage) {
        runOnUiThread(() -> Toast.makeText(this, "Login failed: " + errorMessage, Toast.LENGTH_LONG).show());
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
