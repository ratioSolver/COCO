package it.cnr.coco.utils;

import android.content.Context;
import android.graphics.Rect;

import androidx.annotation.NonNull;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.CameraSelector;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.camera.mlkit.vision.MlKitAnalyzer;
import androidx.camera.view.PreviewView;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleOwner;

import com.google.common.util.concurrent.ListenableFuture;
import com.google.gson.JsonObject;
import com.google.mlkit.vision.face.Face;
import com.google.mlkit.vision.face.FaceDetection;
import com.google.mlkit.vision.face.FaceDetector;
import com.google.mlkit.vision.face.FaceDetectorOptions;

import it.cnr.coco.api.Item;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.Executors;

public class Camera {

    private static final String TAG = "Camera";
    private final Context context;
    private final Item item;
    private final FaceDetector faceDetector;

    public Camera(@NonNull Context context, @NonNull Item item) {
        this.context = context;
        this.item = item;
        FaceDetectorOptions options = new FaceDetectorOptions.Builder()
                .setPerformanceMode(FaceDetectorOptions.PERFORMANCE_MODE_FAST)
                .setClassificationMode(FaceDetectorOptions.CLASSIFICATION_MODE_ALL)
                .enableTracking()
                .build();
        faceDetector = FaceDetection.getClient(options);
    }

    public void startCamera() {
        ListenableFuture<ProcessCameraProvider> providerFuture = ProcessCameraProvider.getInstance(context);
        providerFuture.addListener(() -> {
            try {
                ProcessCameraProvider cameraProvider = providerFuture.get();
                cameraProvider.unbindAll();

                ImageAnalysis analysis = new ImageAnalysis.Builder()
                        .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST).build();

                MlKitAnalyzer analyzer = new MlKitAnalyzer(Collections.singletonList(faceDetector),
                        ImageAnalysis.COORDINATE_SYSTEM_ORIGINAL, ContextCompat.getMainExecutor(context),
                        result -> {
                            List<Face> faces = result.getValue(faceDetector);
                            if (faces == null || faces.isEmpty())
                                return;

                            // Best face is the one with the largest bounding box
                            Face best = Collections.max(faces, Comparator
                                    .comparingInt(f -> f.getBoundingBox().width() * f.getBoundingBox().height()));

                            // Get face properties
                            float smileProb = best.getSmilingProbability();

                            // Normalize to [-1, +1]
                            Rect box = best.getBoundingBox();
                            float normX = ((box.centerX()
                                    / (float) analysis.getResolutionInfo().getResolution().getWidth()) - 0.5f) * 2f;
                            float normY = ((box.centerY()
                                    / (float) analysis.getResolutionInfo().getResolution().getHeight()) - 0.5f) * 2f;

                            JsonObject message = new JsonObject();
                            message.addProperty("smiling", smileProb);
                            message.addProperty("face_x", normX);
                            message.addProperty("face_y", normY);
                            Executors.newSingleThreadExecutor()
                                    .execute(() -> Connection.getInstance().publish(item, message));
                        });
                analysis.setAnalyzer(ContextCompat.getMainExecutor(context), analyzer);

                CameraSelector camera = CameraSelector.DEFAULT_FRONT_CAMERA;
                cameraProvider.bindToLifecycle((LifecycleOwner) context, camera, analysis);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }, ContextCompat.getMainExecutor(context));
    }

    public void destroy() {
        if (faceDetector != null)
            faceDetector.close();
    }
}
