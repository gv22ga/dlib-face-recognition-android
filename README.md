# dlib-face-recognition-android
Recognize faces in android using dlib state-of-the-art face recognition model based on deep learning. The model has an accuracy of 99.13% on the LFW benchmark.

## App
[Video to be added]

## Usage
    git clone --recursive https://github.com/gv22ga/dlib-face-recognition-android.git
Now you can directly open the `dlib-face-recognition-app` in android studio

## Build JNI (Optional)
First get the required libraries
    
    ./envsetup

To build native code and copy to android studio's project manually
    
    cd [dlib-face-recognition-android]
    ndk-build -j 2
    cp -r libs/* dlib-face-recognition-app/dlib/src/main/jniLibs
Please see [dlib-android](https://github.com/tzutalin/dlib-android) for more details

## Dlib 19.9
This project uses dlib 19.9. Due to some c++11 issues, I had problem compiling dlib 19.9 with opencv. So I made some small changes in `dlib/dlib/serialize.h` and `dlib/dlib/dnn/layers.h`  files in dlib library.

## Sample code
```java
FaceRec mFaceRec = new FaceRec(Constants.getDLibDirectoryPath());
List<VisionDetRet> results = mFaceRec.recognize(image_bitmap);
for(VisionDetRet n:results) {
    Log.d(TAG, n.getLabel()); // prints the name of recognized person
}
```

## Contact
If you need any help, you can contact me on email `gv22ga@gmail.com`

## License
[MIT](https://github.com/gv22ga/dlib-face-recognition-android/blob/master/LICENSE)

## Thanks
Many thanks to [tzutalin](https://github.com/tzutalin) for [dlib-android](https://github.com/tzutalin/dlib-android)
