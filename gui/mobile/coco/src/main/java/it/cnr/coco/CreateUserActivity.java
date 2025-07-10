package it.cnr.coco;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import android.widget.EditText;
import android.content.Intent;

public class CreateUserActivity extends Activity implements ConnectionListener {

    @Override
    protected void onCreate(@NonNull Bundle savedInstanceState) {
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
