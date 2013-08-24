#pragma once

#include "ofMain.h"
#include "MessageProperty.h"

#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxOsc.h"
#include "ofxOgsn.h"

//====================
// Structs
//====================
// blob
//----------
struct myBlob {
    unsigned int ID;
    ofImage blobImg;
//    ofImage blobImgAlph;
    ofxCvBlob cvBlob;
    ofColor color;
    float distToCentPos;
//    vector<float> moveProbs;
};

//----------
// Image Parameters
//----------
struct ImageParams {
    ofColor avgColor;
    vector<float> histR;
    vector<float> histG;
    vector<float> histB;
};

//----------
// scan line effect
//----------
struct ScanLine {
    float posY;
    float speed;
    float w;
    ofColor col;
};

/// extra scanning effect
struct ScanLineExtra : public ScanLine {
    float posX;
    float life;
    float secondScanPos;
    float secondScanSpeed;
    vector<float> vec;
};

//----------
// blobs effects
//----------
struct BlobObject {
    ofPoint pos;
    float size;
    float decay;
    float life = 255;
    ofColor col;
};

//----------
// Future Points
//----------
struct FuturePoints {
    ofPoint pts;
    float alph = 255;
};

//----------
// Mapping effects
//----------
static int smMapId = 0;
struct MapCommon {
    MapCommon():mapId(++smMapId), enable(false){};
    const int mapId;
    bool enable;
};

struct MapDownSampl : public MapCommon {
    int resoX = 1;
    int resoY = 1;
};

struct MapHexText : public MapCommon {
    ofPoint offset;
};

struct MapDetectColor : public MapCommon {
    enum _mode { HUE, BRIGHTNESS, SATURATION } mode;
    float speed = 1;
    float threshold = 50;
    float pos = 0;
    float tmpPos = -1;
    ofxCvGrayscaleImage tMapColorBlob;
    ofxCvContourFinder tCont;
};

struct MapReturnScanLine : public MapCommon {
    vector<ScanLine> lines;
};

struct MapDetectBlobs : public MapCommon {
    vector<myBlob> * pBlobs = NULL;
    int blobNum = 0;
    bool isDrawLines;
};

struct MapSortPixels : public MapCommon {
    ofImage dstImg;
};

struct MapTextFile : public MapCommon {
    string text;
};

//---------------------------------------
class Canvas {
public:
    
    Canvas();
    Canvas(const int ID, const ofRectangle& rectangle, const string& sendhost, const int sendpoat);
    virtual ~Canvas();
    
    void setCanbasID(const int i){mCanvasID = i;};
    void setOscSender(const string& host, const int poat);
    void setCanvasSize(const int width, const int height);
    int getCanvasWidth(){ return mCanvasWidth; };
    int getCanvasHeight(){ return mCanvasHeight; };
    void getInfoText(stringstream& s);
    
    void exportEachDitherNoiseWaveData(); //!< export dither noise
    
    //===== Image and Blobs =====
    void drawMarkov();
    void drawDebug();
    void drawInfoText(const int x, const int y);
    void drawOrgImage();
    void drawWhiteBack(const unsigned char brightness);
    void drawColorBack(const ofColor color);
    void drawVirtualGazePoint();
    void testDraw(); //!< for debug
    void setColorImage(const string& path, const bool resetCanvasSize = true);
    void setLabelingImageAndCreateBlobs(const string& path, const bool resetCanvasSize = true);
    void setBlobNum(const int num);
    void setResolution(const int resoX, const int resoY){ mResoNumX = resoX; mResoNumY = resoY; };
    void plusBlobNum();
    void minusBlobNum();
        
    //===== Visual Effects =====
    void updateEffects();
    void drawEffects(const bool overDraw = true);
    int addSubImage(const string& path);
    
    //make effects
    void makeScanLine(const float _speed, const float _w);
    void makeScanEdge(const float _speed, const float _w);
    void makeScanPixels(const float _speed, const float _w);
    void makeScanHisto(const float _speed, const float _w, const float lifeTime);
    void makeScanHisto(const float _pos, const float _speed, const float _w, const float lifeTime);
    void makeScanSpectrum(const float _pos, const float _speed, const float _w, const float lifeTime);
    void makeGazeObjects(const ofPoint& _pos, const float _size, const ofColor _col, const float _decay);
    void detectFuturePoints(ofImage& src_img);
    void ClearAllEffects();
    
    //Mapping Effects draw
    void setBackColor(const ofColor col){ mBackColor = col; };
    void setFrontColor(const ofColor col){ mFrontColor = col; };
    void setMap();
    void setMapDownSample(const int _resoX, const int _resoY);
    void setMapHexText(const ofPoint _offset);
    void setMapDetectColor(const float _speed, MapDetectColor::_mode _mode = MapDetectColor::HUE);
    void setMapReturnScanLine(const float _speed, const float _w);
    void setMapDetectBlobs(const int blobNum, const bool isDrawLines);
    
    //only to sound
    void sendAvgCol();
    void sendHistoR(const int num);
    void sendHistoG(const int num);
    void sendHistoB(const int num);
    
    
    //Draw Image;
    bool bDrawWhite;
    bool bDrawOrgImg;
    bool bDrawLabelImg;
    bitset<10> bDrawSubImg;

    ofImage* mCurrentSubImg;
    
private:
    
    //===== private methods =====
    vector<float> getDitherNoiseWavePts(ofImage& blobImg, int reso, bool isX);
    void exportDitherNoiseWaveData(vector<vector<float> > wavePts, const string& path);
    void getFixedScanParam(const ScanLine& scanLine, int * y1, int * y2, int * scanW);
    ImageParams calcImageParameters(ofImage& img);
    // OSC send message
    void sendOsc(const string& addr, const float arg1);
    void sendOsc(const string& addr, const float arg1, const float arg2);
    void sendOsc(const string& addr, const float arg1, const float arg2, const float arg3);
    void sendOsc(const string& addr, const string& argStr);
    // Draw Mapping
    void drawMapDownSampl();
    void drawMapHexText();
    void drawMapDetectColorBlob();
    void drawMapReturnScanLine();
    void drawMapDetectBlobs();

    //===== Common =====
    int mCanvasID;
    int mCanvasWidth;
    int mCanvasHeight;
    ofxOscSender sender;
    string mSendHost;
    int mSendPort;
    ofxOgsn og;
    
    //===== Image and Blobs =====
    ofImage mOrgImg;
    ofImage mLabelImg;
    vector<myBlob> mMyBlobs;
    int mBlobNum;
    int mMaxBlob;
    int mTmpBlobNum;
    ofPoint mBlobsCenter; ///< 全blobの重心の重心
    vector<ofColor> mBlobColers;
    
    ofPoint mVGazePos;
    bitset<9> bFunction;
        
    // Dither Noise value
    int mResoNumX;
    int mResoNumY;
    vector<float> mDithNoiseX;
    vector<float> mDithNoiseY;
    vector<float> mDithNoiseXComp;
    int mDithNoiseCounter;
    
    
    //===== Visual Effects =====
    vector<ofImage> mSubImg;
//    ofTrueTypeFont mFont;
    
    
public:
    
    ImageParams mImageParams;

    //Scan Effects
    vector<ScanLine> mScanLines;
    vector<ScanLine> mScanEdges;
    vector<ScanLine> mScanPixels;
    vector<ScanLineExtra> mScanHistos;
    vector<ScanLineExtra> mScanSpectrum;
    
    //Blob Object
    vector<BlobObject> mBlobEffects;
    
    //Mapping Effects
    ofColor mBackColor, mFrontColor;
    MapDownSampl mMapDownSample;
    MapHexText mMapHexText;
    MapDetectColor mMapDetectColor;
    MapReturnScanLine mMapReturnScanLine;
    MapDetectBlobs mMapDetectBlobs;

private:
    
    //Blob Object
    vector<BlobObject> mGazeObject;
    
    //Future Points
    vector<ofPoint> mFuturePts;
    
};
