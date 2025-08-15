package it.cnr.coco.utils;

import android.content.Context;
import android.view.View;
import android.widget.ImageView;

import androidx.annotation.NonNull;

import com.bumptech.glide.Glide;

import com.google.gson.JsonObject;

import it.cnr.coco.api.Item;

public class Face implements View.OnClickListener, ConnectionListener {

    private static final String TAG = "Face";

    private final Context context;
    private final Item item;
    private ImageView face;

    public Face(@NonNull Context context, @NonNull Item item, ImageView face) {
        this.context = context;
        this.item = item;
        this.face = face;
        Connection.getInstance().addListener(this); // Register this class as a listener for connection events
    }

    public void setFaceView(@NonNull ImageView face) {
        this.face = face;
        if (item.getValue() != null && item.getValue().data() != null) {
            String faceImage = item.getValue().data().getAsJsonObject().get("face").getAsString();
            Glide.with(context).load(Settings.getInstance().getHost() + "/assets/" + faceImage).into(face);
        }
        face.setOnClickListener(this);
    }

    public void updateFace(String faceImage) {
        if (face != null)
            Glide.with(context).load(Settings.getInstance().getHost() + "/assets/" + faceImage).into(face);
    }

    @Override
    public void onClick(View view) {
        JsonObject message = new JsonObject();
        message.addProperty("listening", true);
        Connection.getInstance().publish(item, message);
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onReceivedMessage(@NonNull JsonObject message) {
        String msgType = message.getAsJsonPrimitive(Connection.MSG_TYPE).getAsString();
        if ("new_data".equals(msgType) && message.getAsJsonPrimitive("id").getAsString().equals(item.getId()))
            if (message.has("face"))
                updateFace(message.getAsJsonPrimitive("face").getAsString());
    }

    @Override
    public void onConnectionFailed(@NonNull String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }

    public void destroy() {
        Connection.getInstance().removeListener(this);
    }
}
