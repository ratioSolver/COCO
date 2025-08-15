package it.cnr.coco.utils;

import java.util.ArrayList;
import java.util.Locale;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.speech.tts.TextToSpeech;
import android.speech.tts.TextToSpeech.OnInitListener;
import android.speech.tts.UtteranceProgressListener;
import android.util.Log;

import androidx.annotation.NonNull;

import com.google.gson.JsonNull;
import com.google.gson.JsonObject;

import it.cnr.coco.api.Item;

import java.util.concurrent.Executors;

public class Language extends UtteranceProgressListener
        implements OnInitListener, RecognitionListener, ConnectionListener {

    private static final String TAG = "Language";
    private static final String SAYING = "saying"; // Key for the saying message
    private static final String UNDERSTOOD = "understood"; // Key for the understood message
    private static final String LISTENING = "listening"; // Key for the listening message

    private final Item item;
    private final TextToSpeech textToSpeech;
    private final SpeechRecognizer speechRecognizer;

    public Language(@NonNull Context context, @NonNull Item item) {
        this.item = item;
        textToSpeech = new TextToSpeech(context, this);
        speechRecognizer = SpeechRecognizer.createSpeechRecognizer(context);
        speechRecognizer.setRecognitionListener(this);
        Connection.getInstance().addListener(this);
    }

    @Override
    public void onReadyForSpeech(Bundle params) {
        Log.d(TAG, "Ready for speech");
    }

    @Override
    public void onBeginningOfSpeech() {
        Log.d(TAG, "Speech started");
    }

    @Override
    public void onRmsChanged(float rmsdB) {
        Log.d(TAG, "RMS changed: " + rmsdB);
    }

    @Override
    public void onBufferReceived(byte[] buffer) {
        Log.d(TAG, "Buffer received: " + buffer.length + " bytes");
    }

    @Override
    public void onEndOfSpeech() {
        Log.d(TAG, "Speech ended");
    }

    @Override
    public void onError(int error) {
        Log.e(TAG, "Error occurred: " + error);
    }

    @Override
    public void onResults(Bundle results) {
        ArrayList<String> matches = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        if (matches != null && !matches.isEmpty()) {
            Log.d(TAG, "Speech results: " + matches.get(0));
            JsonObject message = new JsonObject();
            message.addProperty(UNDERSTOOD, matches.get(0));
            Executors.newSingleThreadExecutor().execute(() -> Connection.getInstance().publish(item, message));
        }
    }

    @Override
    public void onPartialResults(Bundle partialResults) {
        ArrayList<String> matches = partialResults.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        if (matches != null && !matches.isEmpty())
            Log.d(TAG, "Partial results: " + matches.get(0));
    }

    @Override
    public void onEvent(int eventType, Bundle params) {
        Log.d(TAG, "Event occurred: " + eventType);
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
        JsonObject message = new JsonObject();
        message.add(SAYING, "");
        Executors.newSingleThreadExecutor().execute(() -> Connection.getInstance().publish(item, message));
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
        String msgType = message.getAsJsonPrimitive(Connection.MSG_TYPE).getAsString();
        if ("new_data".equals(msgType) && message.getAsJsonPrimitive("id").getAsString().equals(item.getId())) {
            JsonObject value = message.getAsJsonObject("value").get("data").getAsJsonObject();
            if (value.has(SAYING)) {
                String saying = value.getAsJsonPrimitive(SAYING).getAsString();
                Log.d(TAG, "Received saying: " + saying);
                new Handler(Looper.getMainLooper()).post(() -> {
                    if (textToSpeech.speak(saying, TextToSpeech.QUEUE_FLUSH, null,
                            "utterance-" + System.currentTimeMillis()) == TextToSpeech.ERROR)
                        Log.e(TAG, "TextToSpeech speak returned ERROR");
                });
            }
            if (value.has(LISTENING) && value.get(LISTENING).getAsBoolean()) {
                Log.d(TAG, "Listening for speech");
                Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
                intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
                intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault());
                intent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true);
                new Handler(Looper.getMainLooper()).post(() -> speechRecognizer.startListening(intent));
            }
        }
    }

    @Override
    public void onConnectionFailed(@NonNull String errorMessage) {
    }

    @Override
    public void onConnectionClosed() {
    }

    public void destroy() {
        Log.d(TAG, "Destroying Language resources");
        speechRecognizer.destroy();
        textToSpeech.shutdown();
        Connection.getInstance().removeListener(this);
    }
}
