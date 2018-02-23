// Created by Gaurav on Feb 23, 2018

#pragma once

#include <dlib/dnn.h>
#include <dlib/string.h>
#include <jni_common/jni_fileutils.h>
#include <jni_common/jni_utils.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_loader/load_image.h>
#include <glog/logging.h>
#include <jni.h>
#include <memory>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <time.h>
#include <dirent.h>

using namespace dlib;
using namespace std;

// ResNet network copied from dnn_face_recognition_ex.cpp in dlib/examples
template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                    alevel0<
                    alevel1<
                    alevel2<
                    alevel3<
                    alevel4<
                    max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                    input_rgb_image_sized<150>
                    >>>>>>>>>>>>;

class DLibFaceRecognizer {
 private:
  std::string landmark_model;
  std::string model_dir_path;
  std::string image_dir_path;
  std::string dnn_model;
  anet_type net;
  dlib::shape_predictor sp;
  std::unordered_map<int, dlib::full_object_detection> mFaceShapeMap;
  dlib::frontal_face_detector face_detector;
  std::vector<dlib::rectangle> rects;
  std::vector<std::string> rec_names;
  std::vector<matrix<float,0,1>> rec_face_descriptors;
  std::vector<dlib::rectangle> rec_rects;
  std::vector<std::string> rec_labels;
  bool is_training;

  inline void init() {
    LOG(INFO) << "init DLibFaceRecognizer";
    face_detector = dlib::get_frontal_face_detector();
    landmark_model = model_dir_path + "/shape_predictor_5_face_landmarks.dat";
    dnn_model = model_dir_path + "/dlib_face_recognition_resnet_model_v1.dat";
    image_dir_path = model_dir_path + "/images";
    is_training = false;
  }

public:
  inline void train() {
    LOG(INFO) << "train DLibFaceRecognizer";
    struct dirent *entry;
    DIR *dp;

    dp = opendir((image_dir_path).c_str());
    if (dp == NULL) {
        LOG(INFO) << ("Opendir: Path does not exist or could not be read.");
    }

    std::vector<matrix<rgb_pixel>> faces;
    std::vector<std::string> names;

    // load images from dlib image directory and extract faces
    while ((entry = readdir(dp))) {
      std::string filename = entry->d_name;
      if (filename=="." || filename=="..") continue;

      cv::Mat file_image = cv::imread(image_dir_path + "/" + filename, CV_LOAD_IMAGE_COLOR);
      LOG(INFO) << "Load image " << (entry->d_name);
      dlib::cv_image<dlib::bgr_pixel> img(file_image);

      std::vector<dlib::rectangle> frects = face_detector(img);
      if (frects.size()==1) {
        auto face = frects[0];
        auto shape = sp(img, face);
        matrix<rgb_pixel> face_chip;
        extract_image_chip(img, get_face_chip_details(shape,150,0.25), face_chip);
        faces.push_back(move(face_chip));
        names.push_back(filename);
        LOG(INFO) << "Added image " << filename;
      } else if (frects.size()==0) {
        LOG(INFO) << "No face found in image " << filename;
      } else {
        LOG(INFO) << "More than one face found in image " << filename;
      }
    }
    closedir(dp);

    is_training = true;
    // calculate face descriptors and set global vars
    LOG(INFO) << "Calculating face descriptors " << jniutils::currentDateTime();
    rec_face_descriptors = net(faces);
    LOG(INFO) << "Calculated face descriptors  " << jniutils::currentDateTime()<<" Size "<<rec_face_descriptors.size();
    rec_names = names;
    is_training = false;
  }


  DLibFaceRecognizer() { init(); }

  DLibFaceRecognizer(const std::string& dlib_rec_example_dir)
      : model_dir_path(dlib_rec_example_dir) {
    init();
    if (!landmark_model.empty() && jniutils::fileExists(landmark_model) && !dnn_model.empty() && jniutils::fileExists(dnn_model)) {
      // load the model weights
      dlib::deserialize(landmark_model) >> sp;
      dlib::deserialize(dnn_model) >> net;
      LOG(INFO) << "Models loaded";
    }
  }

  inline int rec(const cv::Mat& image) {
    if (is_training) return 0;
    if (image.empty())
      return 0;
    if (image.channels() == 1) {
      cv::cvtColor(image, image, CV_GRAY2BGR);
    }
    CHECK(image.channels() == 3);

    dlib::cv_image<dlib::bgr_pixel> img(image);

    std::vector<matrix<rgb_pixel>> faces;
    std::vector<dlib::rectangle> frects = face_detector(img);
    for (auto face : frects)
    {
      auto shape = sp(img, face);
      matrix<rgb_pixel> face_chip;
      extract_image_chip(img, get_face_chip_details(shape,150,0.25), face_chip);
      faces.push_back(move(face_chip));
    }

    if (faces.size() == 0)
    {
      LOG(INFO) << "No faces found in image!";
    }
    LOG(INFO) << "calculating face descriptor in image..." << jniutils::currentDateTime();
    std::vector<matrix<float,0,1>> face_descriptors = net(faces);
    LOG(INFO) << "face descriptors in camera image calculated   "<<jniutils::currentDateTime()<<" Size "<<face_descriptors.size();

    rec_rects.clear();
    rec_labels.clear();
    for (size_t i = 0; i < face_descriptors.size();  ++i) {
      for (size_t j = 0; j < rec_face_descriptors.size();  ++j) {
        if (length(face_descriptors[i]-rec_face_descriptors[j]) < 0.6) {
          LOG(INFO) << rec_names[j]<<" FOUND!!!!";
          dlib::rectangle r = frects[i];
          rec_rects.push_back(r);
          rec_labels.push_back(rec_names[j]);
        }
      }
    }

    return rec_rects.size();
  }

  virtual inline int det(const cv::Mat& image) {
    if (is_training) return 0;
    if (image.empty())
      return 0;
    if (image.channels() == 1) {
      cv::cvtColor(image, image, CV_GRAY2BGR);
    }
    CHECK(image.channels() == 3);
    // TODO : Convert to gray image to speed up detection
    // It's unnecessary to use color image for face/landmark detection

    dlib::cv_image<dlib::bgr_pixel> img(image);

    std::vector<matrix<rgb_pixel>> faces;
    rects = face_detector(img);
    return rects.size();
  }

  inline std::vector<dlib::rectangle> getRecResultRects() { return rec_rects; }
  inline std::vector<std::string> getRecResultLabels() { return rec_labels; }
  inline std::vector<dlib::rectangle> getDetResultRects() { return rects; }
};


