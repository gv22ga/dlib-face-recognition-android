package com.tzutalin.dlib;

import android.os.Environment;

import java.io.File;

/**
 * Created by darrenl on 2016/4/22.
 * Modified by Gaurav on Feb 23, 2018
 */

public final class Constants {
    private Constants() {
        // Constants should be prive
    }

    public static String getDLibDirectoryPath() {
        File sdcard = Environment.getExternalStorageDirectory();
        String targetPath = sdcard.getAbsolutePath() + File.separator + "dlib_rec_example";
        return targetPath;
    }

    public static String getDLibImageDirectoryPath() {
        String targetPath = getDLibDirectoryPath()+ File.separator + "images";
        return targetPath;
    }

    public static String getFaceShapeModelPath() {
        String targetPath = getDLibDirectoryPath() + File.separator + "shape_predictor_5_face_landmarks.dat";
        return targetPath;
    }

    public static String getFaceDescriptorModelPath() {
        String targetPath = getDLibDirectoryPath() + File.separator + "dlib_face_recognition_resnet_model_v1.dat";
        return targetPath;
    }
}
