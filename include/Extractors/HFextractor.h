#ifndef HFNETEXTRACTOR_H
#define HFNETEXTRACTOR_H

#include <cmath>
#include <vector>
#include <list>
#include <opencv2/opencv.hpp>
#include "Extractors/BaseModel.h"

namespace ORB_SLAM3
{

class BaseModel;
class Settings;

class HFextractor
{
public:

    HFextractor(int nfeatures, Settings* settings, const std::vector<BaseModel*>& vpModels);

    HFextractor(int nfeatures, float scaleFactor, 
                int nlevels, const std::vector<BaseModel*>& vpModels);

    void determineLayers_wo_pyrimid(int _nOctaves, int _nLayersPerOctave, float _scaleFactor, float _initSigma);

    ~HFextractor(){}

    // Compute the features and descriptors on an image.
    int operator()(const cv::Mat &_image, std::vector<cv::KeyPoint>& _keypoints,
                   cv::Mat &_localDescriptors, cv::Mat &_globalDescriptors);

    int inline GetLevels(void) {
        return nlevels;}

    float inline GetScaleFactor(void) {
        return scaleFactor;}

    std::vector<float> inline GetScaleFactors(void) {
        return mvScaleFactor;
    }

    std::vector<float> inline GetInverseScaleFactors(void) {
        return mvInvScaleFactor;
    }

    std::vector<float> inline GetScaleSigmaSquares(void) {
        return mvLevelSigma2;
    }

    std::vector<float> inline GetInverseScaleSigmaSquares(void) {
        return mvInvLevelSigma2;
    }

    std::vector<cv::Mat> mvImagePyramid;
    std::vector<int> mnFeaturesPerLevel;

    int nfeatures;
    float threshold;

    std::vector<BaseModel*> mvpModels;

protected:

    float scaleFactor;
    int nlevels;
    bool bUseOctTree;
    bool bNeedRGB;

    std::vector<float> mvScaleFactor;
    std::vector<float> mvInvScaleFactor;    
    std::vector<float> mvLevelSigma2;
    std::vector<float> mvInvLevelSigma2;

    std::vector<int> umax;

    void ComputePyramid(const cv::Mat &image);

    int ExtractSingleLayer(const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                           cv::Mat &localDescriptors, cv::Mat &globalDescriptors);

    int ExtractMultiLayers(const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                           cv::Mat &localDescriptors, cv::Mat &globalDescriptors);
    
    int ExtractMultiLayersParallel(const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                                   cv::Mat &localDescriptors, cv::Mat &globalDescriptors);
};

} //namespace ORB_SLAM

#endif
