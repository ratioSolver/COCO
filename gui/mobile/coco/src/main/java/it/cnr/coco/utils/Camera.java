package it.cnr.coco.utils;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.camera.lifecycle.ProcessCameraProvider;

import com.google.common.util.concurrent.ListenableFuture;

public class Camera {

    public Camera(@NonNull Context context) {
        ListenableFuture<ProcessCameraProvider> cameraProviderFuture = ProcessCameraProvider.getInstance(context);
    }
}
