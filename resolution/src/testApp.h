#pragma once

#include "ofMain.h"
#include "Canvas.h"
#include "MessageProperty.h"

#include "ofxOsc.h"
#include "ofxCv.h"
#include "ofxXmlSettings.h"
#include "ofxQuadWarp.h"
#include "ofxControlPanel.h"
#include "ofxOgsn.h"

//#define AUDIO_PROCESS //!< audio processing
//#define REC_VIDEO
#define HONBAN

//====================
// save file name
//====================
static const string GUI_SAVE_XML = "gui_settings.xml";
static const string MAP_CALIB_XML = "mapping_settings_A0.xml";
static const string LOG_PATH = string(getenv("HOME")) + "/Desktop/log/" + ofGetTimestampString() + ".txt";

//==================== 
// set path of each image
//====================
static const string LABEL_IMAGE_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/test.bmp";

static const string ORG_IMAGE_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/org.bmp";

static const string EDGE_IMAGE_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/edge.tiff";

static const string DOWN_SAMPLE_4_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/down_sample_4.tiff";

static const string DOWN_SAMPLE_8_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/down_sample_8.tiff";

static const string DOWN_SAMPLE_16_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/down_sample_16.tiff";

static const string DOWN_SAMPLE_32_PATH
= string(getenv("HOME")) + "/Dropbox/MMM2013/image/down_sample_32.tiff";

//====================
// OSC Settings
//====================
static const string RECEIVER_HOST = "127.0.0.1";
static const int RECEIVER_PORT = 52341;

static const string SENDER_HOST = "127.0.0.1";
static const int SENDER_PORT = 52344;
static const string SENDER_HOST_CNV1 = "127.0.0.1";
static const int SENDER_PORT_CNV1 = 52342;
static const string SENDER_HOST_CNV2 = "127.0.0.1";
static const int SENDER_PORT_CNV2 = 52343;

//====================
// Audio Settings
//====================
static const int SAMPLE_RATE = 44100;
static const int BUFFER_SIZE = 512;

//==================== myLog path
static const string MYLOG_PATH
= string(getenv("HOME")) + "/Desktop/log/" + ofGetTimestampString() + ".txt";

//====================
// ControlPanel
//====================
static const string GUI_MASTER_VOLUME = "master_volume";

//====================
// Scene - MMM2013 -
//====================
enum scene {
    SCENE_01 = 0,
    SCENE_02,
    SCENE_03
};

/** mode */
enum _mode {
    DEBUG_MODE = 0,
    CALIB_MODE,
    VISUAL_MODE
};

/** Control mode to Mapping Points */
enum mappingPtsMode {
    C01_TOP_LEFT = 0,
    C01_TOP_RIGHT,
    C01_BOTTOM_LEFT,
    C01_BOTTOM_RIGHT,
    C01_CENTER,
    C02_TOP_LEFT,
    C02_TOP_RIGHT,
    C02_BOTTOM_LEFT,
    C02_BOTTOM_RIGHT,
    C02_CENTER,
    mappingPtsModeNum
};

/** Mapping */
struct quadWarp {
    ofxQuadWarp warper;
    ofFbo fbo;
    ofPoint cornerPt[4];
    void setCorners() {
        warper.setCorner(cornerPt[0], 0);
        warper.setCorner(cornerPt[1], 1);
        warper.setCorner(cornerPt[2], 2);
        warper.setCorner(cornerPt[3], 3);
    }
};

class testApp : public ofBaseApp {
public:
    
    void setup();
    void update();
    void draw();
    void exit();
    
    void drawInfoText();
    void drawCanvas(const int canvasID);
    void drawCalibrationView();
    void drawAudioVisualize(const int x=0, const int y=0, const int w=240, const int h=60);
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void audioOut(float * input, int bufferSize, int nChannels);
    
private:
    
    void drawVirtualGazePoint();
    void setMappingCorners(const int x, const int y);
    
    Canvas mCanvas[2];
        
    bitset<9> bFunction;
    bool bDrawInfoText;
    
    ofPoint mVGazePos;
    
    _mode mode;
    scene mScene;
    ofxOscReceiver receiver;
    ofxOscSender sender;
    
    quadWarp mapping[2];
    mappingPtsMode mMapPtsMode;
    ofxXmlSettings mappingXml;
    
    ofxOgsn og;
    ofxControlPanel gui;
    std::ofstream myLog;
    
    //===== Audio =====
    ofSoundStream soundStream;
    vector <float> mSampleData; //!< audio visualize

    //===== REC Movie =====
#ifdef REC_VIDEO
    ofImage mGrabImg;
    IplImage mCvframe;
    CvVideoWriter * mCvVideoWriter;
#endif
    
};
