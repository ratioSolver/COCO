package it.cnr.coco.utils;

import java.util.ArrayList;
import java.util.Locale;
import java.util.function.Consumer;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.util.Log;

import androidx.annotation.NonNull;

public class CoCoSTT implements RecognitionListener {

    private static final String TAG = "CoCoSTT";
    private final SpeechRecognizer speechRecognizer;
    private final Consumer<String> onTextRecognized;

    public CoCoSTT(@NonNull Context context, @NonNull Consumer<String> onTextRecognized) {
        this.speechRecognizer = android.speech.SpeechRecognizer.createSpeechRecognizer(context);
        this.speechRecognizer.setRecognitionListener(this);
        this.onTextRecognized = onTextRecognized;
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
            onTextRecognized.accept(matches.get(0));
        } else
            onTextRecognized.accept("");
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

    public void startListening() {
        Log.d(TAG, "Starting to listen for speech");
        Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault());
        intent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true);
        speechRecognizer.startListening(intent);
    }

    public void destroy() {
        Log.d(TAG, "Destroying SpeechRecognizer");
        speechRecognizer.destroy();
    }
}
