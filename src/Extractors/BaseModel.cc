#include "Settings.h"
#include "Extractors/BaseModel.h"
#include "Extractors/SIFTextractor.h"
#include "Extractors/SURFextractor.h"
#include "Extractors/KAZEextractor.h"
#include "Extractors/PythonFeatureExtractor.h"

#include <unordered_set>
#include <unordered_map>

using namespace std;

namespace ORB_SLAM3
{

std::vector<BaseModel*> gvpModels;
BaseModel* gpGlobalModel = nullptr;

void InitAllModels(Settings* settings)
{
    //InitAllModels(settings->strModelPath(), settings->modelType(), settings->newImSize(), settings->nLevels(), settings->scaleFactor());
    /*TO EDIT initialize model/s based on custom parameters*/

    if (gvpModels.size())
    {
        for (auto pModel : gvpModels) delete pModel;
        gvpModels.clear();
    }
    bool found;
    ModelType modelType = settings->modelType();
    cv::Size ImSize = settings->newImSize();
    if(settings->bPyrimid())
    {
        // Employ Pyrimid layered extraction
        int nLevels = settings->readParameter<int>(settings->fSettings,"Extractor.nLevels",found);
        float scaleFactor = settings->readParameter<float>(settings->fSettings,"Extractor.scaleFactor",found);
        gvpModels.reserve(nLevels);
        float scale = 1.0f;
        for (int level = 0; level < nLevels; ++level)
        {
            cv::Vec4i inputShape{1, cvRound(ImSize.height * scale), cvRound(ImSize.width * scale), 1};
            BaseModel *pNewModel = nullptr;
            ModelDetectionMode mode;
            if(modelType == oCVSIFTModel){
                if (level == 0) mode = kImageToLocalAndGlobal;
                else mode = kImageToLocal;
                pNewModel = InitSIFTModel(mode, settings);
            }
            else if(modelType == oCVSURFModel){
                if (level == 0) mode = kImageToLocalAndGlobal;
                else mode = kImageToLocal;
                pNewModel = InitSURFModel(mode, settings);
            }
            else if(modelType == oCVKAZEModel){
                if (level == 0) mode = kImageToLocalAndGlobal;
                else mode = kImageToLocal;
                pNewModel = InitKAZEModel(mode, settings);
            }
            else if(modelType == PythonFeatureModel){
                if (level == 0) mode = kImageToLocalAndGlobal;
                else mode = kImageToLocal;
                pNewModel = InitPythonFeature(mode, settings);
            }
            else
            {
                cerr << "Wrong type of model!" << endl;
                exit(-1);
            }
            gvpModels.emplace_back(pNewModel);
            scale /= scaleFactor;
        }
    }
    else
    {
        // Method self employs pyrimid get scale values from extraction method
        gvpModels.reserve(1);
        BaseModel *pNewModel = nullptr;
        ModelDetectionMode mode;
        int level = 0;
        if(modelType == oCVSIFTModel){
            if (level == 0) mode = kImageToLocalAndGlobal;
            else mode = kImageToLocal;
            pNewModel = InitSIFTModel(mode, settings);
        }
        else if(modelType == oCVSURFModel){
            if (level == 0) mode = kImageToLocalAndGlobal;
            else mode = kImageToLocal;
            pNewModel = InitSURFModel(mode, settings);
        }
        else if(modelType == oCVKAZEModel){
            if (level == 0) mode = kImageToLocalAndGlobal;
            else mode = kImageToLocal;
            pNewModel = InitKAZEModel(mode, settings);
        }
        else if(modelType == PythonFeatureModel){
            if (level == 0) mode = kImageToLocalAndGlobal;
            else mode = kImageToLocal;
            pNewModel = InitPythonFeature(mode, settings);
        }
        else
        {
            cerr << "Wrong type of model!" << endl;
            exit(-1);
        }
        gvpModels.emplace_back(pNewModel);
    }
    // Init Global Model
        if (gpGlobalModel) delete gpGlobalModel;

        cv::Vec4i inputShape{1, ImSize.height / 8, ImSize.width / 8, 96};
        BaseModel *pNewModel = nullptr;
        ModelDetectionMode mode;
        if (modelType == oCVSIFTModel)
        {
            pNewModel = nullptr;
        }
        else if (modelType == oCVSURFModel)
        {
            pNewModel = nullptr;
        }
        else if (modelType == oCVKAZEModel)
        {
            pNewModel = nullptr;
        }
        else if (modelType == PythonFeatureModel)
        {
            pNewModel = nullptr;
        }
        else
        {
            cerr << "Wrong type of model!" << endl;
            exit(-1);
        }
        gpGlobalModel = pNewModel;
}


std::vector<BaseModel*> GetModelVec(void)
{
    if (gvpModels.empty())
    {
        cerr << "Try to get models before initialize them" << endl;
        exit(-1);
    }
    return gvpModels;
}

BaseModel* GetGlobalModel(void)
{
    if (gvpModels.empty())
    {
        cerr << "Try to get global model before initialize it" << endl;
        exit(-1);
    }
    return gpGlobalModel;
}

/* TO EDIT BaseModel* InitSIFTModel(Settings* settings)
    chnage initialization to customized opencv perameters
*/
BaseModel* InitSIFTModel(ModelDetectionMode mode, Settings* settings)
{
    BaseModel* pModel;
    bool found = false;
    int nfeatures = settings->nFeatures();

    int nOctaveLayers = settings->readParameter<int>(settings->fSettings,"Extractor.SIFT.nOctaveLayers",found, false);
    if(found)
    {
        cout << "Found Extractor.SIFT.nOctaveLayers in settings" << endl;
    }
    else
    {
        nOctaveLayers = 3;
        cout << "Did not find Extractor.SIFT.nOctaveLayers in settings, using default :" << nOctaveLayers << endl;
    }

    double contrastThreshold = settings->readParameter<double>(settings->fSettings,"Extractor.SIFT.contrastThreshold",found, false);
    if(found)
    {
        cout << "Found Extractor.SIFT.contrastThreshold in settings" << endl;
    }
    else
    {
        contrastThreshold = 0.04;
        cout << "Did not find Extractor.SIFT.contrastThreshold in settings, using default :" << contrastThreshold << endl;
    }

    double edgeThreshold = settings->readParameter<double>(settings->fSettings,"Extractor.SIFT.edgeThreshold",found, false);
    if(found)
    {
        cout << "Found Extractor.SIFT.edgeThreshold in settings" << endl;
    }
    else
    {
        edgeThreshold = 10.0;
        cout << "Did not find Extractor.SIFT.edgeThreshold in settings, using default :" << edgeThreshold << endl;
    }

    double sigma = settings->readParameter<double>(settings->fSettings,"Extractor.SIFT.sigma",found, false);
    if(found)
    {
        cout << "Found Extractor.SIFT.sigma in settings" << endl;
    }
    else
    {
        sigma = 1.6;
        cout << "Did not find Extractor.SIFT.sigma in settings, using default :" << sigma << endl;
    }

    pModel = new SIFTModel(nfeatures, nOctaveLayers, contrastThreshold, edgeThreshold, sigma);
    if (pModel->IsValid())
    {
        cout << "Successfully created OpenCV SIFT."
             << " Mode: " << gStrModelDetectionName[mode]<< endl;
        cout << "nfeatures: " << nfeatures << endl
             << "nOctaveLayers: " << nOctaveLayers << endl
             << "contrastThreshold: " << contrastThreshold << endl
             << "edgeThreshold: " << edgeThreshold << endl
             << "sigma: " << sigma << endl;
    }
    else exit(-1);

    return pModel;
}

/* TO EDIT BaseModel* InitSURFModel(Settings* settings)
    chnage initialization to customized opencv perameters
*/
BaseModel* InitSURFModel(ModelDetectionMode mode, Settings* settings)
{
    BaseModel* pModel;
    bool found = false;

    double hessianThreshold = settings->readParameter<double>(settings->fSettings,"Extractor.SURF.hessianThreshold",found, false);
    if(found)
    {
        cout << "Found Extractor.SURF.hessianThreshold in settings" << endl;
    }
    else
    {
        hessianThreshold = 100.0;
        cout << "Did not find Extractor.SURF.hessianThreshold in settings, using default :" << hessianThreshold << endl;
    }

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

    bool extended = settings->readParameter<int>(settings->fSettings,"Extractor.SURF.extended",found, false) > 0;
    if(found)
    {
        cout << "Found Extractor.SURF.extended in settings" << endl;
    }
    else
    {
        extended = false;
        cout << "Did not find Extractor.SURF.extended in settings, using default :" << extended << endl;
    }

    bool upright = settings->readParameter<int>(settings->fSettings,"Extractor.SURF.upright",found, false) > 0;
    if(found)
    {
        cout << "Found Extractor.SURF.upright in settings" << endl;
    }
    else
    {
        upright = false;
        cout << "Did not find Extractor.SURF.upright in settings, using default :" << upright << endl;
    }

    pModel = new SURFModel(hessianThreshold, nOctaves, nOctaveLayers, extended, upright);
    if (pModel->IsValid())
    {
        cout << "Successfully created OpenCV SURF."
             << " Mode: " << gStrModelDetectionName[mode]<< endl;
        cout    << "hessianThreshold: " << hessianThreshold << endl
                << "nOctaves: " << nOctaves << endl
                << "nOctaveLayers: " << nOctaveLayers << endl
                << "extended: " << extended << endl
                << "upright: " << upright << endl;
    }
    else exit(-1);

    return pModel;
}

/*
BaseModel* InitSURFModel(ModelDetectionMode mode, Settings* settings)
{
    cout << "SURF Feature extractor not implemeted yet" << endl;
    exit(-1);
}
*/

/* TO EDIT BaseModel* InitKAZEModel(Settings* settings)
    chnage initialization to customized opencv perameters
*/
BaseModel* InitKAZEModel(ModelDetectionMode mode, Settings* settings)
{
    BaseModel* pModel;
    bool found = false;

    bool extended = settings->readParameter<int>(settings->fSettings,"Extractor.KAZE.extended",found, false) > 0;
    if(found)
    {
        cout << "Found Extractor.KAZE.extended in settings" << endl;
    }
    else
    {
        extended = false;
        cout << "Did not find Extractor.KAZE.extended in settings, using default :" << extended << endl;
    }

    bool upright = settings->readParameter<int>(settings->fSettings,"Extractor.KAZE.upright",found, false) > 0;
    if(found)
    {
        cout << "Found Extractor.KAZE.upright in settings" << endl;
    }
    else
    {
        upright = false;
        cout << "Did not find Extractor.KAZE.upright in settings, using default :" << upright << endl;
    }

    float threshold = settings->readParameter<float>(settings->fSettings,"Extractor.KAZE.threshold",found, false);
    if(found)
    {
        cout << "Found Extractor.KAZE.threshold in settings" << endl;
    }
    else
    {
        threshold = 0.001;
        cout << "Did not find Extractor.KAZE.threshold in settings, using default :" << threshold << endl;
    }

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

    pModel = new KAZEModel(extended, upright, threshold, nOctaves, nOctaveLayers);
    if (pModel->IsValid())
    {
        cout    << "Successfully created OpenCV KAZE."
                << " Mode: " << gStrModelDetectionName[mode] << endl;
        cout    << "extended: " << extended << endl
                << "upright: " << upright << endl
                << "threshold: " << threshold << endl
                << "nOctaves: " << nOctaves << endl
                << "nOctaveLayers: " << nOctaveLayers << endl;
    }
    else exit(-1);

    return pModel;
}
/*
BaseModel* InitKAZEModel(ModelDetectionMode mode, Settings* settings)
{
    cout << "SURF Feature extractor not implemeted yet" << endl;
    exit(-1);
}
*/

BaseModel* InitPythonFeature(ModelDetectionMode mode, Settings* settings)
{
    BaseModel* pModel;
    bool found = false;
    int nfeatures = settings->nFeatures();

    pModel = new PythonFeature(nfeatures);

    if (pModel->IsValid())
    {
        cout    << "Successfully created pyslam SuperPoint."
                << " Mode: " << gStrModelDetectionName[mode] << endl;
        cout    << "nfeatures: " << nfeatures << endl;
    }
    else exit(-1);

    return pModel;
}

void ExtractorNode::DivideNode(ExtractorNode &n1, ExtractorNode &n2, ExtractorNode &n3, ExtractorNode &n4)
{
    const int halfX = ceil(static_cast<float>(UR.x-UL.x)/2);
    const int halfY = ceil(static_cast<float>(BR.y-UL.y)/2);

    //Define boundaries of childs
    n1.UL = UL;
    n1.UR = cv::Point2i(UL.x+halfX,UL.y);
    n1.BL = cv::Point2i(UL.x,UL.y+halfY);
    n1.BR = cv::Point2i(UL.x+halfX,UL.y+halfY);
    n1.vKeys.reserve(vKeys.size());

    n2.UL = n1.UR;
    n2.UR = UR;
    n2.BL = n1.BR;
    n2.BR = cv::Point2i(UR.x,UL.y+halfY);
    n2.vKeys.reserve(vKeys.size());

    n3.UL = n1.BL;
    n3.UR = n1.BR;
    n3.BL = BL;
    n3.BR = cv::Point2i(n1.BR.x,BL.y);
    n3.vKeys.reserve(vKeys.size());

    n4.UL = n3.UR;
    n4.UR = n2.BR;
    n4.BL = n3.BR;
    n4.BR = BR;
    n4.vKeys.reserve(vKeys.size());

    //Associate points to childs
    for(size_t i=0;i<vKeys.size();i++)
    {
        const cv::KeyPoint &kp = vKeys[i];
        if(kp.pt.x<n1.UR.x)
        {
            if(kp.pt.y<n1.BR.y)
                n1.vKeys.push_back(kp);
            else
                n3.vKeys.push_back(kp);
        }
        else if(kp.pt.y<n1.BR.y)
            n2.vKeys.push_back(kp);
        else
            n4.vKeys.push_back(kp);
    }

    if(n1.vKeys.size()==1)
        n1.bNoMore = true;
    if(n2.vKeys.size()==1)
        n2.bNoMore = true;
    if(n3.vKeys.size()==1)
        n3.bNoMore = true;
    if(n4.vKeys.size()==1)
        n4.bNoMore = true;

}

static bool compareNodes(pair<int,ExtractorNode*>& e1, pair<int,ExtractorNode*>& e2){
    if(e1.first < e2.first){
        return true;
    }
    else if(e1.first > e2.first){
        return false;
    }
    else{
        if(e1.second->UL.x < e2.second->UL.x){
            return true;
        }
        else{
            return false;
        }
    }
}

std::vector<cv::KeyPoint> DistributeOctTree(const std::vector<cv::KeyPoint>& vToDistributeKeys, const int minX,
                                            const int maxX, const int minY, const int maxY, const int N)
{
    // Compute how many initial nodes
    const int nIni = round(static_cast<float>(maxX-minX)/(maxY-minY));

    const float hX = static_cast<float>(maxX-minX)/nIni;

    list<ExtractorNode> lNodes;

    vector<ExtractorNode*> vpIniNodes;
    vpIniNodes.resize(nIni);

    for(int i=0; i<nIni; i++)
    {
        ExtractorNode ni;
        ni.UL = cv::Point2i(hX*static_cast<float>(i),0);
        ni.UR = cv::Point2i(hX*static_cast<float>(i+1),0);
        ni.BL = cv::Point2i(ni.UL.x,maxY-minY);
        ni.BR = cv::Point2i(ni.UR.x,maxY-minY);
        ni.vKeys.reserve(vToDistributeKeys.size());

        lNodes.push_back(ni);
        vpIniNodes[i] = &lNodes.back();
    }

    //Associate points to childs
    for(size_t i=0;i<vToDistributeKeys.size();i++)
    {
        const cv::KeyPoint &kp = vToDistributeKeys[i];
        vpIniNodes[kp.pt.x/hX]->vKeys.push_back(kp);
    }

    list<ExtractorNode>::iterator lit = lNodes.begin();

    while(lit!=lNodes.end())
    {
        if(lit->vKeys.size()==1)
        {
            lit->bNoMore=true;
            lit++;
        }
        else if(lit->vKeys.empty())
            lit = lNodes.erase(lit);
        else
            lit++;
    }

    bool bFinish = false;

    int iteration = 0;

    vector<pair<int,ExtractorNode*> > vSizeAndPointerToNode;
    vSizeAndPointerToNode.reserve(lNodes.size()*4);

    while(!bFinish)
    {
        iteration++;

        int prevSize = lNodes.size();

        lit = lNodes.begin();

        int nToExpand = 0;

        vSizeAndPointerToNode.clear();

        while(lit!=lNodes.end())
        {
            if(lit->bNoMore)
            {
                // If node only contains one point do not subdivide and continue
                lit++;
                continue;
            }
            else
            {
                // If more than one point, subdivide
                ExtractorNode n1,n2,n3,n4;
                lit->DivideNode(n1,n2,n3,n4);

                // Add childs if they contain points
                if(n1.vKeys.size()>0)
                {
                    lNodes.push_front(n1);
                    if(n1.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n1.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }
                if(n2.vKeys.size()>0)
                {
                    lNodes.push_front(n2);
                    if(n2.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n2.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }
                if(n3.vKeys.size()>0)
                {
                    lNodes.push_front(n3);
                    if(n3.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n3.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }
                if(n4.vKeys.size()>0)
                {
                    lNodes.push_front(n4);
                    if(n4.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n4.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }

                lit=lNodes.erase(lit);
                continue;
            }
        }

        // Finish if there are more nodes than required features
        // or all nodes contain just one point
        if((int)lNodes.size()>=N || (int)lNodes.size()==prevSize)
        {
            bFinish = true;
        }
        else if(((int)lNodes.size()+nToExpand*3)>N)
        {

            while(!bFinish)
            {

                prevSize = lNodes.size();

                vector<pair<int,ExtractorNode*> > vPrevSizeAndPointerToNode = vSizeAndPointerToNode;
                vSizeAndPointerToNode.clear();

                sort(vPrevSizeAndPointerToNode.begin(),vPrevSizeAndPointerToNode.end(),compareNodes);
                for(int j=vPrevSizeAndPointerToNode.size()-1;j>=0;j--)
                {
                    ExtractorNode n1,n2,n3,n4;
                    vPrevSizeAndPointerToNode[j].second->DivideNode(n1,n2,n3,n4);

                    // Add childs if they contain points
                    if(n1.vKeys.size()>0)
                    {
                        lNodes.push_front(n1);
                        if(n1.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n1.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
                    if(n2.vKeys.size()>0)
                    {
                        lNodes.push_front(n2);
                        if(n2.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n2.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
                    if(n3.vKeys.size()>0)
                    {
                        lNodes.push_front(n3);
                        if(n3.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n3.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
                    if(n4.vKeys.size()>0)
                    {
                        lNodes.push_front(n4);
                        if(n4.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n4.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }

                    lNodes.erase(vPrevSizeAndPointerToNode[j].second->lit);

                    if((int)lNodes.size()>=N)
                        break;
                }

                if((int)lNodes.size()>=N || (int)lNodes.size()==prevSize)
                    bFinish = true;

            }
        }
    }

    // Retain the best point in each node
    vector<cv::KeyPoint> vResultKeys;
    vResultKeys.reserve(N);
    for(list<ExtractorNode>::iterator lit=lNodes.begin(); lit!=lNodes.end(); lit++)
    {
        vector<cv::KeyPoint> &vNodeKeys = lit->vKeys;
        cv::KeyPoint* pKP = &vNodeKeys[0];
        float maxResponse = pKP->response;

        for(size_t k=1;k<vNodeKeys.size();k++)
        {
            if(vNodeKeys[k].response>maxResponse)
            {
                pKP = &vNodeKeys[k];
                maxResponse = vNodeKeys[k].response;
            }
        }

        vResultKeys.push_back(*pKP);
    }

    return vResultKeys;
}

// copy from tensorflow.contrib.resampler
void Resampler(const float* data, const float* warp, float* output,
               const int batch_size, const int data_height, 
               const int data_width, const int data_channels, const int num_sampling_points)
{
    const int warp_batch_stride = num_sampling_points * 2;
    const int data_batch_stride = data_height * data_width * data_channels;
    const int output_batch_stride = num_sampling_points * data_channels;
    const float zero = static_cast<float>(0.0);
    const float one = static_cast<float>(1.0);

    auto resample_batches = [&](const int start, const int limit) {
      for (int batch_id = start; batch_id < limit; ++batch_id) {
        // Utility lambda to access data point and set output values.
        // The functions take care of performing the relevant pointer
        // arithmetics abstracting away the low level details in the
        // main loop over samples. Note that data is stored in NHWC format.
        auto set_output = [&](const int sample_id, const int channel,
                              const float value) {
          output[batch_id * output_batch_stride + sample_id * data_channels +
                 channel] = value;
        };

        auto get_data_point = [&](const int x, const int y, const int chan) {
          const bool point_is_in_range =
              (x >= 0 && y >= 0 && x <= data_width - 1 && y <= data_height - 1);
          return point_is_in_range
                     ? data[batch_id * data_batch_stride +
                            data_channels * (y * data_width + x) + chan]
                     : zero;
        };

        for (int sample_id = 0; sample_id < num_sampling_points; ++sample_id) {
          const float x = warp[batch_id * warp_batch_stride + sample_id * 2];
          const float y = warp[batch_id * warp_batch_stride + sample_id * 2 + 1];
          // The interpolation function:
          // a) implicitly pads the input data with 0s (hence the unusual checks
          // with {x,y} > -1)
          // b) returns 0 when sampling outside the (padded) image.
          // The effect is that the sampled signal smoothly goes to 0 outside
          // the original input domain, rather than presenting a jump
          // discontinuity at the image boundaries.
          if (x > static_cast<float>(-1.0) && y > static_cast<float>(-1.0) &&
              x < static_cast<float>(data_width) &&
              y < static_cast<float>(data_height)) {
            // Precompute floor (f) and ceil (c) values for x and y.
            const int fx = std::floor(static_cast<float>(x));
            const int fy = std::floor(static_cast<float>(y));
            const int cx = fx + 1;
            const int cy = fy + 1;
            const float dx = static_cast<float>(cx) - x;
            const float dy = static_cast<float>(cy) - y;

            for (int chan = 0; chan < data_channels; ++chan) {
              const float img_fxfy = dx * dy * get_data_point(fx, fy, chan);
              const float img_cxcy =
                  (one - dx) * (one - dy) * get_data_point(cx, cy, chan);
              const float img_fxcy = dx * (one - dy) * get_data_point(fx, cy, chan);
              const float img_cxfy = (one - dx) * dy * get_data_point(cx, fy, chan);
              set_output(sample_id, chan,
                         img_fxfy + img_cxcy + img_fxcy + img_cxfy);
            }
          } else {
            for (int chan = 0; chan < data_channels; ++chan) {
              set_output(sample_id, chan, zero);
            }
          }
        }
      }
    };
    
    resample_batches(0, batch_size);
}

std::vector<cv::KeyPoint> NMS(const std::vector<cv::KeyPoint> &vToDistributeKeys, int width, int height, int radius)
{
    std::vector<std::vector<const cv::KeyPoint*>> vpKeypoints(height, vector<const cv::KeyPoint*>(width, nullptr));
    unordered_set<const cv::KeyPoint*> selected;
    for (const auto &kpt : vToDistributeKeys)
    {
        vpKeypoints[kpt.pt.y][kpt.pt.x] = &kpt;
        selected.insert(&kpt);
    }

    for (const auto &kpt : vToDistributeKeys)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            for (int dy = -radius; dy <= radius; ++dy)
            {
                const int x = kpt.pt.x + dx;
                const int y = kpt.pt.y + dy;
                if (x < 0 || y < 0 || x >= width || y >= height) continue;
                if (!vpKeypoints[y][x]) continue;

                if (vpKeypoints[kpt.pt.y][kpt.pt.x]->response < vpKeypoints[y][x]->response)
                {
                    selected.erase(vpKeypoints[kpt.pt.y][kpt.pt.x]);
                    vpKeypoints[kpt.pt.y][kpt.pt.x] = nullptr;
                    goto jump;
                }
            }
        }
        jump: ;
    }

    std::vector<cv::KeyPoint> res;
    res.reserve(selected.size());
    for (const auto &pKP : selected)
    {
        res.emplace_back(*pKP);
    }
    return res;
}

}
