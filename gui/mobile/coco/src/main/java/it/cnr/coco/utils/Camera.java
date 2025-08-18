package it.cnr.coco.utils;

import android.content.Context;
import android.graphics.Rect;

import androidx.annotation.NonNull;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageProxy;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleOwner;

import com.google.common.util.concurrent.ListenableFuture;
import com.google.mlkit.vision.common.InputImage;
import com.google.mlkit.vision.face.Face;
import com.google.mlkit.vision.face.FaceDetection;
import com.google.mlkit.vision.face.FaceDetector;
import com.google.mlkit.vision.face.FaceDetectorOptions;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class Camera {

    private final FaceDetector faceDetector;

    public Camera() {
        FaceDetectorOptions options = new FaceDetectorOptions.Builder()
                .setPerformanceMode(FaceDetectorOptions.PERFORMANCE_MODE_FAST)
                .setClassificationMode(FaceDetectorOptions.CLASSIFICATION_MODE_ALL)
                .enableTracking()
                .build();
        faceDetector = FaceDetection.getClient(options);
    }

    public void startCamera(@NonNull Context context) {
        ListenableFuture<ProcessCameraProvider> providerFuture = ProcessCameraProvider.getInstance(context);
        providerFuture.addListener(() -> {
            try {
                ProcessCameraProvider cameraProvider = providerFuture.get();
                cameraProvider.unbindAll();

                ImageAnalysis analysis = new ImageAnalysis.Builder()
                        .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST).build();

                analysis.setAnalyzer(ContextCompat.getMainExecutor(context), new ImageAnalysis.Analyzer() {
                    @Override
                    public void analyze(@NonNull ImageProxy imageProxy) {
                        // Get image dimensions
                        int width = imageProxy.getWidth();
                        int height = imageProxy.getHeight();

                        // Convert ImageProxy to InputImage for ML Kit
                        @SuppressWarnings("UnsafeOptInUsageError")
                        InputImage inputImage = InputImage.fromMediaImage(imageProxy.getImage(),
                                imageProxy.getImageInfo().getRotationDegrees());

                        // Run ML Kit face detection
                        faceDetector.process(inputImage).addOnSuccessListener(faces -> {
                            if (faces.isEmpty()) {
                                imageProxy.close();
                                return;
                            }
                            // Best face is the one with the largest bounding box
                            Face bestFace = Collections.max(faces, Comparator
                                    .comparingInt(f -> f.getBoundingBox().width() * f.getBoundingBox().height()));

                            // Normalize to [-1, +1]
                            Rect box = bestFace.getBoundingBox();
                            float normX = ((box.centerX() / (float) width) - 0.5f) * 2f;
                            float normY = ((box.centerY() / (float) height) - 0.5f) * 2f;

                            imageProxy.close();
                        }).addOnFailureListener(e -> imageProxy.close());
                    }
                });

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
