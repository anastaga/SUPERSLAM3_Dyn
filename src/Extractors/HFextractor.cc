
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <iostream>

#include "Extractors/HFextractor.h"
#include "Settings.h"

using namespace cv;
using namespace std;

namespace ORB_SLAM3
{

const int PATCH_SIZE = 31;
const int HALF_PATCH_SIZE = 15;
const int EDGE_THRESHOLD = 19;

HFextractor::HFextractor(int _nfeatures, Settings* settings, const std::vector<BaseModel*>& _vpModels):
    nfeatures(_nfeatures), mvpModels(_vpModels)
{
    bNeedRGB = settings->bNeedRGB();
    bool found = false;
    if(mvpModels[0]->Type() == oCVSIFTModel){
        Size im_size = settings->newImSize();
        int _noctaves = cvRound(std::log( (double)std::min( im_size.height, im_size.width ) ) / std::log(2.) - 2) - 1.0;
        cout << "num Octaves Detectd from imsize " << _noctaves << endl;

        int nOctaveLayers = settings->readParameter<int>(settings->fSettings,"Extractor.SIFT.nOctaveLayers",found, false);
        if(found)
        {
            cout << "Found Extractor.SIFT.nOctaveLayers in settings" << endl;
        }
        else
        {
            nOctaveLayers = 3;
            cout << "Did not find Extractor.SIFT.nOctaveLayers in settings, using default " << nOctaveLayers << endl;
        }

        double sigma = settings->readParameter<double>(settings->fSettings,"Extractor.SIFT.sigma",found, false);
        if(found)
        {
            cout << "Found Extractor.SIFT.sigma in settings" << endl;
        }
        else
        {
            sigma = 1.6;
            cout << "Did not find Extractor.SIFT.sigma in settings, using default " << sigma << endl;
        }

        
        determineLayers_wo_pyrimid(_noctaves, 1, 2.0, sigma);
    }
    else if (mvpModels[0]->Type() == oCVSURFModel)
    {
        int nOctaves = settings->readParameter<int>(settings->fSettings,"Extractor.SURF.nOctaves",found, false);
        if(found)
        {
            cout << "Found Extractor.SURF.nOctaves in settings" << endl;
        }
        else
        {
            nOctaves = 4;
            cout << "Did not find Extractor.SURF.nOctaves in settings, using default :" << nOctaves << endl;
        }

        int nOctaveLayers = settings->readParameter<int>(settings->fSettings,"Extractor.SURF.nOctaveLayers",found, false);
        if(found)
        {
            cout << "Found Extractor.SURF.nOctaveLayers in settings" << endl;
        }
        else
        {
            nOctaveLayers = 3;
            cout << "Did not find Extractor.SURF.nOctaveLayers in settings, using default :" << nOctaveLayers << endl;
        }

        determineLayers_wo_pyrimid(nOctaves + 1, 1, 2.0, 1.2);
    }
    else if (mvpModels[0]->Type() == oCVKAZEModel)
    {
        int nOctaves = settings->readParameter<int>(settings->fSettings,"Extractor.KAZE.nOctaves",found, false);
        if(found)
        {
            cout << "Found Extractor.KAZE.nOctaves in settings" << endl;
        }
        else
        {
            nOctaves = 4;
            cout << "Did not find Extractor.KAZE.nOctaves in settings, using default :" << nOctaves << endl;
        }

        int nOctaveLayers = settings->readParameter<int>(settings->fSettings,"Extractor.KAZE.nOctaveLayers",found, false);
        if(found)
        {
            cout << "Found Extractor.KAZE.nOctaveLayers in settings" << endl;
        }
        else
        {
            nOctaveLayers = 4;
            cout << "Did not find Extractor.KAZE.nOctaveLayers in settings, using default :" << nOctaveLayers << endl;
        }

        determineLayers_wo_pyrimid(nOctaves, 1, 2.0, 1.0);
    }
    else if (mvpModels[0]->Type() == PythonFeatureModel)
    {
        cout << "Calculating Scale Values "<< endl;
        determineLayers_wo_pyrimid(5, 1, 1.2, 1.0);
        cout << "Scale values dalculated "<< endl;
    }
    else
    {
        cerr << "Wrong type of model!" << endl;
        exit(-1);
    }
    
}


HFextractor::HFextractor(int _nfeatures, float _scaleFactor, 
                        int _nlevels, const std::vector<BaseModel*>& _vpModels):
        nfeatures(_nfeatures), mvpModels(_vpModels)
{
    scaleFactor = _scaleFactor;
    nlevels = _nlevels;
    mvScaleFactor.resize(nlevels);
    mvLevelSigma2.resize(nlevels);
    mvScaleFactor[0]=1.0f;
    mvLevelSigma2[0]=1.0f;
    for(int i=1; i<nlevels; i++)
    {
        mvScaleFactor[i]=mvScaleFactor[i-1]*scaleFactor;
        mvLevelSigma2[i]=mvScaleFactor[i]*mvScaleFactor[i];
    }

    mvInvScaleFactor.resize(nlevels);
    mvInvLevelSigma2.resize(nlevels);
    for(int i=0; i<nlevels; i++)
    {
        mvInvScaleFactor[i]=1.0f/mvScaleFactor[i];
        mvInvLevelSigma2[i]=1.0f/mvLevelSigma2[i];
    }

    mvImagePyramid.resize(nlevels);

    mnFeaturesPerLevel.resize(nlevels);
    float factor = 1.0f / scaleFactor;
    float nDesiredFeaturesPerScale = nfeatures*(1 - factor)/(1 - (float)pow((double)factor, (double)nlevels));

    int sumFeatures = 0;
    for( int level = 0; level < nlevels-1; level++ )
    {
        mnFeaturesPerLevel[level] = cvRound(nDesiredFeaturesPerScale);
        sumFeatures += mnFeaturesPerLevel[level];
        nDesiredFeaturesPerScale *= factor;
    }
    mnFeaturesPerLevel[nlevels-1] = std::max(nfeatures - sumFeatures, 0);

    //This is for orientation
    // pre-compute the end of a row in a circular patch
    umax.resize(HALF_PATCH_SIZE + 1);

    int v, v0, vmax = cvFloor(HALF_PATCH_SIZE * sqrt(2.f) / 2 + 1);
    int vmin = cvCeil(HALF_PATCH_SIZE * sqrt(2.f) / 2);
    const double hp2 = HALF_PATCH_SIZE*HALF_PATCH_SIZE;
    for (v = 0; v <= vmax; ++v)
        umax[v] = cvRound(sqrt(hp2 - v * v));

    // Make sure we are symmetric
    for (v = HALF_PATCH_SIZE, v0 = 0; v >= vmin; --v)
    {
        while (umax[v0] == umax[v0 + 1])
            ++v0;
        umax[v] = v0;
        ++v0;
    }
}

void HFextractor::determineLayers_wo_pyrimid(int _nOctaves, int _nLayersPerOctave, float _scaleFactor, float _initSigma){
    // (use_pyrimid) == false constructor
    nlevels = _nLayersPerOctave*(_nOctaves + 1);
    scaleFactor = _scaleFactor;//pow(2.0,1.0/(3))  ;
    mvScaleFactor.resize(nlevels);
    mvLevelSigma2.resize(nlevels);

    mvScaleFactor[0]=1.0f;
    mvLevelSigma2[0]=_initSigma;

    for(int i=1; i<nlevels; i++)
    {
        mvScaleFactor[i]=mvScaleFactor[i-1]*scaleFactor;
        mvLevelSigma2[i]=mvLevelSigma2[i-1]*mvScaleFactor[i]*mvScaleFactor[i];
    }

    mvInvScaleFactor.resize(nlevels);
    mvInvLevelSigma2.resize(nlevels);
    for(int i=0; i<nlevels; i++)
    {
        mvInvScaleFactor[i]=1.0f/mvScaleFactor[i];
        mvInvLevelSigma2[i]=1.0f/mvLevelSigma2[i];
    }

    mvImagePyramid.resize(1);
    mnFeaturesPerLevel.resize(1);
    mnFeaturesPerLevel[0] = nfeatures;

    //This is for orientation
    // pre-compute the end of a row in a circular patch
    umax.resize(HALF_PATCH_SIZE + 1);

    int v, v0, vmax = cvFloor(HALF_PATCH_SIZE * sqrt(2.f) / 2 + 1);
    int vmin = cvCeil(HALF_PATCH_SIZE * sqrt(2.f) / 2);
    const double hp2 = HALF_PATCH_SIZE*HALF_PATCH_SIZE;
    for (v = 0; v <= vmax; ++v)
        umax[v] = cvRound(sqrt(hp2 - v * v));

    // Make sure we are symmetric
    for (v = HALF_PATCH_SIZE, v0 = 0; v >= vmin; --v)
    {
        while (umax[v0] == umax[v0 + 1])
            ++v0;
        umax[v] = v0;
        ++v0;
    }
}


int HFextractor::operator() (const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                             cv::Mat &localDescriptors, cv::Mat &globalDescriptors)
{
    if (image.empty()){
        cout << "Image empty" << endl;
        return -1;
    }
    if (bNeedRGB)
    {
        if(image.type() != CV_8UC3) 
        {
            cout << "Image not of type CV_8UC3" << endl;
            return -1;
        }
    }
    else
    {
        if(image.type() != CV_8UC1) 
        {
            cout << "Image not of type CV_8UC1" << endl;
            return -1;
        }
    }
    
    int res = -1;
    if (mvpModels.size() == 1) {
        res = ExtractSingleLayer(image, vKeyPoints, localDescriptors, globalDescriptors);
    }
    else
    {
    	res = ExtractMultiLayers(image, vKeyPoints, localDescriptors, globalDescriptors);
    }
    return res;
}

void HFextractor::ComputePyramid(const cv::Mat &image)
{
    mvImagePyramid[0] = image;
    for (int level = 1; level < nlevels; ++level)
    {
        float scale = mvInvScaleFactor[level];
        Size sz(cvRound((float)image.cols*scale), cvRound((float)image.rows*scale));

        // Compute the resized image
        if( level != 0 )
        {
            resize(mvImagePyramid[level-1], mvImagePyramid[level], sz, 0, 0, INTER_LINEAR);
        }
    }
}

int HFextractor::ExtractSingleLayer(const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                                    cv::Mat &localDescriptors, cv::Mat &globalDescriptors)
{
    if (!mvpModels[0]->Detect(image, vKeyPoints, localDescriptors, globalDescriptors, nfeatures, threshold))
        cerr << "Error while detecting keypoints" << endl;

    return vKeyPoints.size();
}

int HFextractor::ExtractMultiLayers(const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                                    cv::Mat &localDescriptors, cv::Mat &globalDescriptors)
{
    ComputePyramid(image);

    int nKeypoints = 0;
    vector<vector<cv::KeyPoint>> allKeypoints(nlevels);
    vector<cv::Mat> allDescriptors(nlevels);
    for (int level = 0; level < nlevels; ++level)
    {
        if (level == 0)
        {
            if (!mvpModels[level]->Detect(mvImagePyramid[level], allKeypoints[level], allDescriptors[level], globalDescriptors, mnFeaturesPerLevel[level], threshold))
                cerr << "Error while detecting keypoints" << endl;
        }
        else
        {
            if (!mvpModels[level]->Detect(mvImagePyramid[level], allKeypoints[level], allDescriptors[level], mnFeaturesPerLevel[level], threshold))
                cerr << "Error while detecting keypoints" << endl;
        }
        nKeypoints += allKeypoints[level].size();
    }
    vKeyPoints.clear();
    vKeyPoints.reserve(nKeypoints);
    int levels_erased = 0;
    for (int level = 0; level < nlevels; ++level)
    {
        for (auto keypoint : allKeypoints[level])
        {
            keypoint.octave = level;
            keypoint.pt *= mvScaleFactor[level];
            vKeyPoints.emplace_back(keypoint);
        }
        
        if(allDescriptors[level].rows == 0){
	    allDescriptors.erase(allDescriptors.begin() + (level - levels_erased));
	    levels_erased++;
	}
    }
    cv::vconcat(allDescriptors.data(), allDescriptors.size(), localDescriptors);

    return vKeyPoints.size();
}
class DetectParallel : public cv::ParallelLoopBody
{
public:

    DetectParallel (vector<cv::KeyPoint> *allKeypoints, cv::Mat *allDescriptors, cv::Mat *globalDescriptors, HFextractor* pExtractor)
        : mAllKeypoints(allKeypoints), mAllDescriptors(allDescriptors), mGlobalDescriptors(globalDescriptors), mpExtractor(pExtractor) {}

    virtual void operator ()(const cv::Range& range) const CV_OVERRIDE
    {
        for (int level = range.start; level != range.end; ++level)
        {
            if (level == 0)
            {
                if (!mpExtractor->mvpModels[level]->Detect(mpExtractor->mvImagePyramid[level], mAllKeypoints[level], mAllDescriptors[level], *mGlobalDescriptors, mpExtractor->mnFeaturesPerLevel[level], mpExtractor->threshold))
                    cerr << "Error while detecting keypoints" << endl;
            }
            else
            {
                if (!mpExtractor->mvpModels[level]->Detect(mpExtractor->mvImagePyramid[level], mAllKeypoints[level], mAllDescriptors[level], mpExtractor->mnFeaturesPerLevel[level], mpExtractor->threshold))
                    cerr << "Error while detecting keypoints" << endl;
            }
        }
    }

    DetectParallel& operator=(const DetectParallel &) {
        return *this;
    };
private:
    vector<cv::KeyPoint> *mAllKeypoints;
    cv::Mat *mAllDescriptors;
    cv::Mat *mGlobalDescriptors;
    HFextractor* mpExtractor;
};

int HFextractor::ExtractMultiLayersParallel(const cv::Mat &image, std::vector<cv::KeyPoint>& vKeyPoints,
                                            cv::Mat &localDescriptors, cv::Mat &globalDescriptors)
{
    ComputePyramid(image);

    int nKeypoints = 0;
    vector<vector<cv::KeyPoint>> allKeypoints(nlevels);
    vector<cv::Mat> allDescriptors(nlevels);
    
    DetectParallel detector(allKeypoints.data(), allDescriptors.data(), &globalDescriptors, this);
    cv::parallel_for_(cv::Range(0, nlevels), detector);

    for (int level = 0; level < nlevels; ++level)
        nKeypoints += allKeypoints[level].size();

    vKeyPoints.clear();
    vKeyPoints.reserve(nKeypoints);
    for (int level = 0; level < nlevels; ++level)
    {
        for (auto keypoint : allKeypoints[level])
        {
            keypoint.octave = level;
            keypoint.pt *= mvScaleFactor[level];
            vKeyPoints.emplace_back(keypoint);
        }
    }
    cv::vconcat(allDescriptors.data(), allDescriptors.size(), localDescriptors);

    return vKeyPoints.size();
}

} //namespace ORB_SLAM3
