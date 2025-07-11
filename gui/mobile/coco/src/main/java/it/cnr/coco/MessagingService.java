package it.cnr.coco;

import android.util.Log;

import androidx.annotation.NonNull;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

public class MessagingService extends FirebaseMessagingService {

    private static final String TAG = "MessagingService";

    @Override
    public void onNewToken(@NonNull String fcm_token) {
        super.onNewToken(fcm_token);
        Log.d(TAG, "New token: " + fcm_token);
        String token = getSharedPreferences(Connection.COCO_CONNECTION, MODE_PRIVATE).getString("token", null);
        if (token != null)
            Connection.getInstance().newToken(token, fcm_token);
    }

    @Override
    public void onMessageReceived(@NonNull RemoteMessage remoteMessage) {
        super.onMessageReceived(remoteMessage);
    }
}
