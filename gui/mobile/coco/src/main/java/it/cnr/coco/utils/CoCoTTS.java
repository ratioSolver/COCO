package it.cnr.coco.utils;

import android.content.Context;
import android.speech.tts.TextToSpeech;
import android.speech.tts.TextToSpeech.OnInitListener;
import android.speech.tts.UtteranceProgressListener;
import android.util.Log;

import androidx.annotation.NonNull;

import com.google.gson.JsonObject;

import java.util.Locale;

import it.cnr.coco.Connection;
import it.cnr.coco.ConnectionListener;

public class CoCoTTS extends UtteranceProgressListener implements OnInitListener, ConnectionListener {

    private static final String TAG = "CoCoTTS";
    private final TextToSpeech textToSpeech;

    public CoCoTTS(@NonNull Context context) {
        textToSpeech = new TextToSpeech(context, this);
        Connection.getInstance().addListener(this);
    }

    @Override
    public void onInit(int status) {
        if (status == TextToSpeech.SUCCESS) {
            int result = textToSpeech.setLanguage(Locale.getDefault());
            if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED)
                Log.e(TAG, "Language not supported or missing data");
            else {
                Log.d(TAG, "TextToSpeech initialized successfully");
                textToSpeech.setOnUtteranceProgressListener(this);
            }
        } else
            Log.e(TAG, "TextToSpeech initialization failed");
    }

    @Override
    public void onStart(String utteranceId) {
        Log.d(TAG, "TextToSpeech started: " + utteranceId);
    }

    @Override
    public void onDone(String utteranceId) {
        Log.d(TAG, "TextToSpeech done: " + utteranceId);
    }

    @Override
    public void onError(String utteranceId) {
        Log.e(TAG, "TextToSpeech error: " + utteranceId);
    }

    @Override
    public void onConnectionEstablished() {
    }

    @Override
    public void onReceivedMessage(@NonNull JsonObject message) {
    }

    @Override
    public void onConnectionFailed(@NonNull String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }

    public void speak(@NonNull String text) {
        Log.d(TAG, "Speaking: " + text);
        if (textToSpeech.speak(text, TextToSpeech.QUEUE_FLUSH, null,
                "utterance-" + System.currentTimeMillis()) == TextToSpeech.ERROR)
            Log.e(TAG, "Error speaking text: " + text);
    }

    public void destroy() {
        Log.d(TAG, "Destroying TextToSpeech");
        textToSpeech.shutdown();
        Connection.getInstance().removeListener(this);
    }
}
