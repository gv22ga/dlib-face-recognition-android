/*
 *  Created on: Oct 20, 2015
 *      Author: Tzutalin
 *
 *  Copyright (c) 2015 Tzutalin. All rights reserved.
 */
// Modified by Gaurav on Feb 23, 2018

#include <android/bitmap.h>
#include <jni_common/jni_bitmap2mat.h>
#include <jni_common/jni_primitives.h>
#include <jni_common/jni_fileutils.h>
#include <jni_common/jni_utils.h>
#include <recognizer.h>
#include <jni.h>

using namespace cv;

extern JNI_VisionDetRet* g_pJNI_VisionDetRet;

namespace {

#define JAVA_NULL 0
using RecPtr = DLibFaceRecognizer*;

class JNI_FaceRec {
 public:
  JNI_FaceRec(JNIEnv* env) {
    jclass clazz = env->FindClass(CLASSNAME_FACE_REC);
    mNativeContext = env->GetFieldID(clazz, "mNativeFaceRecContext", "J");
    env->DeleteLocalRef(clazz);
  }

  RecPtr getRecognizerPtrFromJava(JNIEnv* env, jobject thiz) {
    RecPtr const p = (RecPtr)env->GetLongField(thiz, mNativeContext);
    return p;
  }

  void setRecognizerPtrToJava(JNIEnv* env, jobject thiz, jlong ptr) {
    env->SetLongField(thiz, mNativeContext, ptr);
  }

  jfieldID mNativeContext;
};

// Protect getting/setting and creating/deleting pointer between java/native
std::mutex gLock;

std::shared_ptr<JNI_FaceRec> getJNI_FaceRec(JNIEnv* env) {
  static std::once_flag sOnceInitflag;
  static std::shared_ptr<JNI_FaceRec> sJNI_FaceRec;
  std::call_once(sOnceInitflag, [env]() {
    sJNI_FaceRec = std::make_shared<JNI_FaceRec>(env);
  });
  return sJNI_FaceRec;
}

RecPtr const getRecPtr(JNIEnv* env, jobject thiz) {
  std::lock_guard<std::mutex> lock(gLock);
  return getJNI_FaceRec(env)->getRecognizerPtrFromJava(env, thiz);
}

// The function to set a pointer to java and delete it if newPtr is empty
void setRecPtr(JNIEnv* env, jobject thiz, RecPtr newPtr) {
  std::lock_guard<std::mutex> lock(gLock);
  RecPtr oldPtr = getJNI_FaceRec(env)->getRecognizerPtrFromJava(env, thiz);
  if (oldPtr != JAVA_NULL) {
    DLOG(INFO) << "setMapManager delete old ptr : " << oldPtr;
    delete oldPtr;
  }

  if (newPtr != JAVA_NULL) {
    DLOG(INFO) << "setMapManager set new ptr : " << newPtr;
  }

  getJNI_FaceRec(env)->setRecognizerPtrToJava(env, thiz, (jlong)newPtr);
}

}  // end unnamespace

#ifdef __cplusplus
extern "C" {
#endif


#define DLIB_FACE_JNI_METHOD(METHOD_NAME) \
  Java_com_tzutalin_dlib_FaceRec_##METHOD_NAME

void JNIEXPORT
    DLIB_FACE_JNI_METHOD(jniNativeClassInit)(JNIEnv* env, jclass _this) {}

jobjectArray getRecResult(JNIEnv* env, RecPtr faceRecognizer,
                             const int& size) {
  LOG(INFO) << "getRecResult";
  jobjectArray jDetRetArray = JNI_VisionDetRet::createJObjectArray(env, size);
  for (int i = 0; i < size; i++) {
    jobject jDetRet = JNI_VisionDetRet::createJObject(env);
    env->SetObjectArrayElement(jDetRetArray, i, jDetRet);
    dlib::rectangle rect = faceRecognizer->getRecResultRects()[i];
    std::string label = faceRecognizer->getRecResultLabels()[i];
    g_pJNI_VisionDetRet->setRect(env, jDetRet, rect.left(), rect.top(),
                                 rect.right(), rect.bottom());
    g_pJNI_VisionDetRet->setLabel(env, jDetRet, label);
  }
  return jDetRetArray;
}

jobjectArray getDetResult(JNIEnv* env, RecPtr faceRecognizer,
                             const int& size) {
  LOG(INFO) << "getDetResult";
  jobjectArray jDetRetArray = JNI_VisionDetRet::createJObjectArray(env, size);
  for (int i = 0; i < size; i++) {
    jobject jDetRet = JNI_VisionDetRet::createJObject(env);
    env->SetObjectArrayElement(jDetRetArray, i, jDetRet);
    dlib::rectangle rect = faceRecognizer->getDetResultRects()[i];
    std::string label = "face";
    g_pJNI_VisionDetRet->setRect(env, jDetRet, rect.left(), rect.top(),
                                 rect.right(), rect.bottom());
    g_pJNI_VisionDetRet->setLabel(env, jDetRet, label);
  }
  return jDetRetArray;
}

JNIEXPORT jobjectArray JNICALL
    DLIB_FACE_JNI_METHOD(jniBitmapDetect)(JNIEnv* env, jobject thiz,
                                          jobject bitmap) {
  LOG(INFO) << "jniBitmapFaceDet";
  cv::Mat rgbaMat;
  cv::Mat bgrMat;
  jniutils::ConvertBitmapToRGBAMat(env, bitmap, rgbaMat, true);
  cv::cvtColor(rgbaMat, bgrMat, cv::COLOR_RGBA2BGR);
  RecPtr mRecPtr = getRecPtr(env, thiz);
  jint size = mRecPtr->det(bgrMat);
  LOG(INFO) << "det face size: " << size;
  return getDetResult(env, mRecPtr, size);
}

JNIEXPORT jobjectArray JNICALL
    DLIB_FACE_JNI_METHOD(jniBitmapRec)(JNIEnv* env, jobject thiz,
                                          jobject bitmap) {
  LOG(INFO) << "jniBitmapFaceDet";
  cv::Mat rgbaMat;
  cv::Mat bgrMat;
  jniutils::ConvertBitmapToRGBAMat(env, bitmap, rgbaMat, true);
  cv::cvtColor(rgbaMat, bgrMat, cv::COLOR_RGBA2BGR);
  RecPtr mRecPtr = getRecPtr(env, thiz);
  jint size = mRecPtr->rec(bgrMat);
  LOG(INFO) << "rec face size: " << size;
  return getRecResult(env, mRecPtr, size);
}

jint JNIEXPORT JNICALL DLIB_FACE_JNI_METHOD(jniInit)(JNIEnv* env, jobject thiz,
                                                     jstring jDirPath) {
  LOG(INFO) << "jniInit";
  std::string dirPath = jniutils::convertJStrToString(env, jDirPath);
  RecPtr mRecPtr = new DLibFaceRecognizer(dirPath);
  setRecPtr(env, thiz, mRecPtr);
  return JNI_OK;
}

jint JNIEXPORT JNICALL DLIB_FACE_JNI_METHOD(jniTrain)(JNIEnv* env, jobject thiz) {
  LOG(INFO) << "jniTrain";
  RecPtr mRecPtr = getRecPtr(env, thiz);
  mRecPtr->train();
  return JNI_OK;
}

jint JNIEXPORT JNICALL
    DLIB_FACE_JNI_METHOD(jniDeInit)(JNIEnv* env, jobject thiz) {
  LOG(INFO) << "jniDeInit";
  setRecPtr(env, thiz, JAVA_NULL);
  return JNI_OK;
}

#ifdef __cplusplus
}
#endif
