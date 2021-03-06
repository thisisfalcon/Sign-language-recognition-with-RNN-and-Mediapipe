// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mediapipe/calculators/tflite/tflite_tensors_to_landmarks_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "tensorflow/lite/interpreter.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

//Edit your mediapipe code because of using input_path&output_path 
extern string input_video_new;
extern string output_video_new;


namespace mediapipe {

// A calculator for converting TFLite tensors from regression models into
// landmarks. Note that if the landmarks in the tensor has more than 3
// dimensions, only the first 3 dimensions will be converted to x,y,z.
//
// Input:
//  TENSORS - Vector of TfLiteTensor of type kTfLiteFloat32. Only the first
//            tensor will be used. The size of the values must be
//            (num_dimension x num_landmarks).
// Output:
//  LANDMARKS(optional) - Result MediaPipe landmarks.
//  NORM_LANDMARKS(optional) - Result MediaPipe normalized landmarks.
//
// Notes:
//   To output normalized landmarks, user must provide the original input image
//   size to the model using calculator option input_image_width and
//   input_image_height.
// Usage example:
// node {
//   calculator: "TfLiteTensorsToLandmarksCalculator"
//   input_stream: "TENSORS:landmark_tensors"
//   output_stream: "LANDMARKS:landmarks"
//   output_stream: "NORM_LANDMARKS:landmarks"
//   options: {
//     [mediapipe.TfLiteTensorsToLandmarksCalculatorOptions.ext] {
//       num_landmarks: 21
//
//       input_image_width: 256
//       input_image_height: 256
//     }
//   }
// }
class TfLiteTensorsToLandmarksCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;

 private:
  ::mediapipe::Status LoadOptions(CalculatorContext* cc);
  int num_landmarks_ = 0;

  ::mediapipe::TfLiteTensorsToLandmarksCalculatorOptions options_;
};
REGISTER_CALCULATOR(TfLiteTensorsToLandmarksCalculator);

::mediapipe::Status TfLiteTensorsToLandmarksCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(!cc->Inputs().GetTags().empty());
  RET_CHECK(!cc->Outputs().GetTags().empty());

  if (cc->Inputs().HasTag("TENSORS")) {
    cc->Inputs().Tag("TENSORS").Set<std::vector<TfLiteTensor>>();
  }

  if (cc->Outputs().HasTag("LANDMARKS")) {
    cc->Outputs().Tag("LANDMARKS").Set<LandmarkList>();
  }

  if (cc->Outputs().HasTag("NORM_LANDMARKS")) {
    cc->Outputs().Tag("NORM_LANDMARKS").Set<NormalizedLandmarkList>();
  }

  return ::mediapipe::OkStatus();
}

::mediapipe::Status TfLiteTensorsToLandmarksCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  MP_RETURN_IF_ERROR(LoadOptions(cc));

  if (cc->Outputs().HasTag("NORM_LANDMARKS")) {
    RET_CHECK(options_.has_input_image_height() &&
              options_.has_input_image_width())
        << "Must provide input with/height for getting normalized landmarks.";
  }
  if (cc->Outputs().HasTag("LANDMARKS") &&
      (options_.flip_vertically() || options_.flip_horizontally())) {
    RET_CHECK(options_.has_input_image_height() &&
              options_.has_input_image_width())
        << "Must provide input with/height for using flip_vertically option "
           "when outputing landmarks in absolute coordinates.";
  }
  return ::mediapipe::OkStatus();
}

::mediapipe::Status TfLiteTensorsToLandmarksCalculator::Process(
    CalculatorContext* cc) {
  if (cc->Inputs().Tag("TENSORS").IsEmpty()) {
    return ::mediapipe::OkStatus();
  }

  const auto& input_tensors =
      cc->Inputs().Tag("TENSORS").Get<std::vector<TfLiteTensor>>();

  const TfLiteTensor* raw_tensor = &input_tensors[0];

  int num_values = 1;
  for (int i = 0; i < raw_tensor->dims->size; ++i) {
    num_values *= raw_tensor->dims->data[i];
  }
  const int num_dimensions = num_values / num_landmarks_;
  CHECK_GT(num_dimensions, 0);

  const float* raw_landmarks = raw_tensor->data.f;

  LandmarkList output_landmarks;

  for (int ld = 0; ld < num_landmarks_; ++ld) {
    const int offset = ld * num_dimensions;
    Landmark* landmark = output_landmarks.add_landmark();

    if (options_.flip_horizontally()) {
      landmark->set_x(options_.input_image_width() - raw_landmarks[offset]);
    } else {
      landmark->set_x(raw_landmarks[offset]);
    }
    if (num_dimensions > 1) {
      if (options_.flip_vertically()) {
        landmark->set_y(options_.input_image_height() -
                        raw_landmarks[offset + 1]);
      } else {
        landmark->set_y(raw_landmarks[offset + 1]);
      }
    }
    if (num_dimensions > 2) {
      landmark->set_z(raw_landmarks[offset + 2]);
    }
  }

  // Output normalized landmarks if required.
  if (cc->Outputs().HasTag("NORM_LANDMARKS")) {
    NormalizedLandmarkList output_norm_landmarks;
    // for (const auto& landmark : output_landmarks) {
    
    //Edit your code
    //To get video name from input_file path
    string video_fname="";
    bool isTrue=false;
    int slx;
    //To locate the video file name in the input path
    for(slx=input_video_new.size()-1;input_video_new[slx]!='/';slx--){
        if(isTrue)
            video_fname=input_video_new[slx]+video_fname;
        if(input_video_new[slx]=='.') isTrue=true;
    }
    slx--;
    //To get directory name that store text file from input_file path
    string dir_name="/";      
    for(;input_video_new[slx]!='/';slx--){
        dir_name=input_video_new[slx]+dir_name;
    }
    //아웃풋 파일의 경로의 일부를 저장하는 변수
    //parameter that store a part of output_file path
    string output_path_cp="";
    int j=0;
    for (;output_video_new[j]!='=';j++);
    j++;
    for(;output_video_new[j]!='_';j++){
        output_path_cp.push_back(output_video_new[j]);
    }
    //output file path 
    string str=output_path_cp+dir_name+video_fname+".txt";
      
    //output file open
    ofstream out(str,std::ios_base::out | std::ios_base::app);

    for (int i = 0; i < output_landmarks.landmark_size(); ++i) {
      const Landmark& landmark = output_landmarks.landmark(i);
      NormalizedLandmark* norm_landmark = output_norm_landmarks.add_landmark();
      norm_landmark->set_x(static_cast<float>(landmark.x()) /
                           options_.input_image_width());
      norm_landmark->set_y(static_cast<float>(landmark.y()) /
                           options_.input_image_height());
      norm_landmark->set_z(landmark.z() / options_.normalize_z());
      
      
      out<<(static_cast<float>(landmark.x()) / options_.input_image_width())<<" ";
      out<<(static_cast<float>(landmark.y()) / options_.input_image_height())<<" ";

    }
    out.close();
    //
    //cout<<cc->InputTimestamp()<<"!\n";
    cc->Outputs()
        .Tag("NORM_LANDMARKS")
        .AddPacket(MakePacket<NormalizedLandmarkList>(output_norm_landmarks)
                       .At(cc->InputTimestamp()));
  }

  // Output absolute landmarks.
  if (cc->Outputs().HasTag("LANDMARKS")) {
    cc->Outputs()
        .Tag("LANDMARKS")
        .AddPacket(MakePacket<LandmarkList>(output_landmarks)
                       .At(cc->InputTimestamp()));
  }

  return ::mediapipe::OkStatus();
}

::mediapipe::Status TfLiteTensorsToLandmarksCalculator::LoadOptions(
    CalculatorContext* cc) {
  // Get calculator options specified in the graph.
  options_ =
      cc->Options<::mediapipe::TfLiteTensorsToLandmarksCalculatorOptions>();
  num_landmarks_ = options_.num_landmarks();

  return ::mediapipe::OkStatus();
}
}  // namespace mediapipe
