#include "testApp.h"

static int myLogCount = 6000;
//using namespace ofxCv;
//using namespace cv;
using namespace myMess;

// tmp values
static scene tmpScene;

//--------------------------------------------------------------
void testApp::setup(){
    ofSetFrameRate(60);
    ofBackground(0, 0, 0);
    ofSetColor(255, 255, 255);
    ofSetVerticalSync(true);
//    ofLog(OF_LOG_VERBOSE);
    
    mode = DEBUG_MODE;
    mScene = SCENE_01;
    
    for (int i = 0; i < bFunction.size(); i++)
        bFunction = false;
    bDrawInfoText = false;
    
    mVGazePos.set(0, 0);
    
    receiver.setup(RECEIVER_PORT);
    myLog.open(LOG_PATH.c_str());

    receiver.setup(RECEIVER_PORT);
    
    myLog.open(MYLOG_PATH.c_str());
    
#ifdef REC_VIDEO
    mCvVideoWriter = cvCreateVideoWriter("test.avi", -1, 10.0, cvSize(ofGetWidth(), ofGetHeight()), 1);
#endif
    
    //----------
    // ControlPanel setup
    //----------
    gui.setup("GUI", 0, 0, 240, 200);
    gui.setXMLFilename(GUI_SAVE_XML);
    gui.addPanel("ctl1", 1);
    gui.addLabel("AUDIO SETTINGS");
    gui.addSlider(GUI_MASTER_VOLUME, GUI_MASTER_VOLUME, 0.2, 0.0, 1.0, false);
    gui.loadSettings(GUI_SAVE_XML);
    gui.setMinimized(true);
        
    //----------
    // Canvas setup
    //----------
    mCanvas[0].setCanbasID(0);
    mCanvas[1].setCanbasID(1);
    mCanvas[0].setLabelingImageAndCreateBlobs(LABEL_IMAGE_PATH, true);
    mCanvas[0].setColorImage(ORG_IMAGE_PATH);
    mCanvas[1].setLabelingImageAndCreateBlobs(LABEL_IMAGE_PATH, true);
    mCanvas[1].setColorImage(ORG_IMAGE_PATH);
    
    mCanvas[1].addSubImage(EDGE_IMAGE_PATH);
    mCanvas[1].addSubImage(DOWN_SAMPLE_4_PATH);
    mCanvas[1].addSubImage(DOWN_SAMPLE_8_PATH);
    mCanvas[1].addSubImage(DOWN_SAMPLE_16_PATH);
    mCanvas[1].addSubImage(DOWN_SAMPLE_32_PATH);

    mCanvas[0].setOscSender(SENDER_HOST_CNV1, SENDER_PORT_CNV1);
    mCanvas[1].setOscSender(SENDER_HOST_CNV2, SENDER_PORT_CNV2);
    
    //----------
    // Canvas Mapping
    //----------
    mMapPtsMode = C01_TOP_LEFT;
    mapping[0].fbo.allocate(mCanvas[0].getCanvasWidth(), mCanvas[0].getCanvasHeight());
    mapping[1].fbo.allocate(mCanvas[1].getCanvasWidth(), mCanvas[1].getCanvasHeight());
    mapping[0].warper.setSourceRect(ofRectangle(0, 0, mCanvas[0].getCanvasWidth(), mCanvas[0].getCanvasHeight()));
    mapping[1].warper.setSourceRect(ofRectangle(0, 0, mCanvas[1].getCanvasWidth(), mCanvas[1].getCanvasHeight()));
    
    //init setting
    ofPoint initPt[2] = {ofPoint(ofGetWidth()/4, ofGetHeight()/2), ofPoint((ofGetWidth()/4)*3, ofGetHeight()/2) };
    
    if (mappingXml.loadFile(MAP_CALIB_XML)) {
        
        mapping[0].cornerPt[0].set(mappingXml.getValue("map_cnv0_c0_x", initPt[0].x - (mCanvas[0].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv0_c0_y", initPt[0].y - (mCanvas[0].getCanvasHeight()/2)));
        mapping[0].cornerPt[1].set(mappingXml.getValue("map_cnv0_c1_x", initPt[0].x + (mCanvas[0].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv0_c1_y", initPt[0].y - (mCanvas[0].getCanvasHeight()/2)));
        mapping[0].cornerPt[2].set(mappingXml.getValue("map_cnv0_c2_x", initPt[0].x + (mCanvas[0].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv0_c2_y", initPt[0].y + (mCanvas[0].getCanvasHeight()/2)));
        mapping[0].cornerPt[3].set(mappingXml.getValue("map_cnv0_c3_x", initPt[0].x - (mCanvas[0].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv0_c3_y", initPt[0].y + (mCanvas[0].getCanvasHeight()/2)));
        mapping[0].setCorners();
        mapping[1].cornerPt[0].set(mappingXml.getValue("map_cnv1_c0_x", initPt[1].x - (mCanvas[1].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv1_c0_y", initPt[1].y - (mCanvas[1].getCanvasHeight()/2)));
        mapping[1].cornerPt[1].set(mappingXml.getValue("map_cnv1_c1_x", initPt[1].x + (mCanvas[1].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv1_c1_y", initPt[1].y - (mCanvas[1].getCanvasHeight()/2)));
        mapping[1].cornerPt[2].set(mappingXml.getValue("map_cnv1_c2_x", initPt[1].x + (mCanvas[1].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv1_c2_y", initPt[1].y + (mCanvas[1].getCanvasHeight()/2)));
        mapping[1].cornerPt[3].set(mappingXml.getValue("map_cnv1_c3_x", initPt[1].x - (mCanvas[1].getCanvasWidth()/2)),
                                   mappingXml.getValue("map_cnv1_c3_y", initPt[1].y + (mCanvas[1].getCanvasHeight()/2)));
        mapping[1].setCorners();
    
    } else {

        mapping[0].cornerPt[0].set(initPt[0].x - (mCanvas[0].getCanvasWidth()/2), initPt[0].y - (mCanvas[0].getCanvasHeight()/2));
        mapping[0].cornerPt[1].set(initPt[0].x + (mCanvas[0].getCanvasWidth()/2), initPt[0].y - (mCanvas[0].getCanvasHeight()/2));
        mapping[0].cornerPt[2].set(initPt[0].x + (mCanvas[0].getCanvasWidth()/2), initPt[0].y + (mCanvas[0].getCanvasHeight()/2));
        mapping[0].cornerPt[3].set(initPt[0].x - (mCanvas[0].getCanvasWidth()/2), initPt[0].y + (mCanvas[0].getCanvasHeight()/2));
        mapping[0].setCorners();
        mapping[1].cornerPt[0].set(initPt[1].x - (mCanvas[1].getCanvasWidth()/2), initPt[1].y - (mCanvas[1].getCanvasHeight()/2));
        mapping[1].cornerPt[1].set(initPt[1].x + (mCanvas[1].getCanvasWidth()/2), initPt[1].y - (mCanvas[1].getCanvasHeight()/2));
        mapping[1].cornerPt[2].set(initPt[1].x + (mCanvas[1].getCanvasWidth()/2), initPt[1].y + (mCanvas[1].getCanvasHeight()/2));
        mapping[1].cornerPt[3].set(initPt[1].x - (mCanvas[1].getCanvasWidth()/2), initPt[1].y + (mCanvas[1].getCanvasHeight()/2));
        mapping[1].setCorners();
        
    }
    
    //----------
    //Audio setup
    //----------
#ifdef AUDIO_PROCESS
    mSampleData.assign(BUFFER_SIZE, 0.0);
    soundStream.listDevices();
    soundStream.setDeviceID(1);
    soundStream.setup(this, 1, 0, SAMPLE_RATE, BUFFER_SIZE, 4);
#endif
    
    // setup sender
    sender.setup(SENDER_HOST, SENDER_PORT);
    ofxOscMessage m;
    m.setAddress("/start");
    sender.sendMessage(m);
}

//--------------------------------------------------------------
void testApp::update(){
    myLogCount--;
    if (myLogCount < 0) {
        myLog << ofGetTimestampString() << " fps:" << ofGetFrameRate() << endl;
        myLogCount = 6000;
    }
    
    //==========
    // OSC Message Reciever
    //==========
    while (receiver.hasWaitingMessages()) {
        ofxOscMessage m;
        receiver.getNextMessage(&m);
        if (m.getRemoteIp() != RECEIVER_HOST) {
            cout << "[ERROR:OSCreceive] bad host: " << m.getRemoteIp() << endl;
            continue;
        }
        vector<string> ms = ofSplitString(m.getAddress().substr(1), "/");
        if (ms.size() <= 0) {
            cout << "[ERROR:OSCReceive] bad address type: " << m.getAddress() << endl;
            continue;
        }
        int i = -1;
        if (ms[0] == CANVAS_01) {
            i = 0;
        } else if (ms[0] == CANVAS_02) {
            i = 1;
        } else {
            cout << "[ERROR:OSCRecieve] bad address" << endl;
            continue;
        }
        
        //----------
        // Color Panel
        //----------
        if (ms[1] == BACK_COLOR_MAP) mCanvas[i].setBackColor
            (ofColor(m.getArgAsInt32(0), m.getArgAsInt32(1), m.getArgAsInt32(2), m.getArgAsInt32(3)));
        if (ms[1] == FRONT_COLOR_MAP) mCanvas[i].setFrontColor
            (ofColor(m.getArgAsInt32(0), m.getArgAsInt32(1), m.getArgAsInt32(2), m.getArgAsInt32(3)));
        
        //----------
        // Draw Image
        //----------
        if (ms[1] == DRAW_ORG_IMAGE) mCanvas[i].bDrawOrgImg = m.getArgAsInt32(0);
        if (ms[1] == DRAW_LABEL_IMAGE) mCanvas[i].bDrawLabelImg = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_01) mCanvas[i].bDrawSubImg[0] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_02) mCanvas[i].bDrawSubImg[1] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_03) mCanvas[i].bDrawSubImg[2] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_04) mCanvas[i].bDrawSubImg[3] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_05) mCanvas[i].bDrawSubImg[4] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_06) mCanvas[i].bDrawSubImg[5] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_07) mCanvas[i].bDrawSubImg[6] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_08) mCanvas[i].bDrawSubImg[7] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_09) mCanvas[i].bDrawSubImg[8] = m.getArgAsInt32(0);
        if (ms[1] == DRAW_SUB_IMAGE_10) mCanvas[i].bDrawSubImg[9] = m.getArgAsInt32(0);
        
        //----------
        // Scan Line
        //----------
        if (ms[1] == VFX_SCAN_LINE) mCanvas[i].makeScanLine(m.getArgAsInt32(0), m.getArgAsInt32(1));
        if (ms[1] == VFX_SCAN_EDGE) mCanvas[i].makeScanEdge(m.getArgAsInt32(0), m.getArgAsInt32(1));
        if (ms[1] == VFX_SCAN_PIX) mCanvas[i].makeScanPixels(m.getArgAsInt32(0), m.getArgAsInt32(1));
        if (ms[1] == VFX_SCAN_HISTO) mCanvas[i].makeScanHisto(m.getArgAsInt32(0), m.getArgAsInt32(1), m.getArgAsInt32(2));
        if (ms[1] == VFX_SCAN_SPECTRUM) mCanvas[i].makeScanSpectrum(m.getArgAsInt32(0), m.getArgAsInt32(1),
                                                                    m.getArgAsInt32(2), m.getArgAsInt32(3));
        if (mCanvas[0].mMapReturnScanLine.enable) {
            if (ms[1] == VFX_SCAN_SPECT_AT_RETLINE) mCanvas[i].makeScanSpectrum(mCanvas[0].mMapReturnScanLine.lines[0].posY,
                                                                                m.getArgAsInt32(0), m.getArgAsInt32(1), m.getArgAsInt32(2));
        }
        
        // all clear
        if (ms[1] == VFX_SCAN_ALL_CLEAR) mCanvas[i].ClearAllEffects();
        
        //----------
        // Mapping Effect
        //----------
        if (ms[1] == VFX_MAP_SET) mCanvas[i].setMap();
        if (ms[1] == VFX_MAP_DOWN_SAMPLE) mCanvas[i].setMapDownSample(m.getArgAsInt32(0), m.getArgAsInt32(1));
        if (ms[1] == VFX_MAP_HEX_TEXT) mCanvas[i].setMapHexText(ofPoint(m.getArgAsInt32(0), m.getArgAsInt32(1)));
        if (ms[1] == VFX_MAP_DETECT_COLOR) mCanvas[i].setMapDetectColor(m.getArgAsInt32(0));
        if (ms[1] == VFX_MAP_RETURN_SCAN_LINE) mCanvas[i].setMapReturnScanLine(m.getArgAsInt32(0), m.getArgAsInt32(1));
        if (ms[1] == VFX_MAP_DETECT_BLOBS) mCanvas[i].setMapDetectBlobs(m.getArgAsInt32(0), m.getArgAsInt32(1));
        
        //----------
        // Image Parameter Request
        //----------
        if (ms[1] == REQ_AVG_COLOR) mCanvas[i].sendAvgCol();
        if (ms[1] == REQ_HISTO_R) mCanvas[i].sendHistoR(m.getArgAsInt32(0));
        if (ms[1] == REQ_HISTO_G) mCanvas[i].sendHistoG(m.getArgAsInt32(0));
        if (ms[1] == REQ_HISTO_B) mCanvas[i].sendHistoB(m.getArgAsInt32(0));
        
    }
    
    mCanvas[0].updateEffects();
    mCanvas[1].updateEffects();
    gui.update();
    
    //show mouse
    if (mode == VISUAL_MODE) {
        ofHideCursor();
    } else {
        ofShowCursor();
    }

}

//--------------------------------------------------------------

void testApp::drawInfoText(){
    //----------
    //情報（テキスト）を描画
    //----------
    stringstream ss;
    ss << "fps           : " << ofGetFrameRate() << endl;
    ss << "display width : " << ofGetWidth() << endl;
    ss << "display height: " << ofGetHeight() << endl;
    ss << "mouse x       : " << ofGetMouseX() << endl;
    ss << "mouse y       : " << ofGetMouseY() << endl;
    ss << "SCENE         : " << mScene << endl;
    ss << endl;
    mCanvas[0].getInfoText(ss);
    ss << endl;
    mCanvas[1].getInfoText(ss);
    
    ofSetColor(255);
    ofDrawBitmapString(ss.str(), 20, 20);
}

/** @brief 擬似注視点を描画 */
void testApp::drawVirtualGazePoint()
{
    float nowX = mouseX;
    float nowY = mouseY;
    //固視微動の再現のため、ちょっとだけランダムに座標ずらす
    nowX += ofRandom(-5, 5);
    nowY += ofRandom(-5, 5);
    //スムーズに遷移
    float tmpX = (nowX - mVGazePos.x) / 3;
    float tmpY = (nowY - mVGazePos.y) / 3;
    mVGazePos.x += tmpX;
    mVGazePos.y += tmpY;
    ofFill();
    ofSetColor(0, 255, 0);
    ofCircle(mVGazePos, 8);
}

//--------------------------------------------------------------
void testApp::draw(){
    ofBackground(0);
    
    //----------
    // 各Canvasを描画
    //----------
    ofPushStyle();
    ofEnableAlphaBlending();
    
    mapping[0].fbo.begin();
    {
        drawCanvas(0);
    }
    mapping[0].fbo.end();
    
    mapping[1].fbo.begin();
    {
        drawCanvas(1);
    }
    mapping[1].fbo.end();
    
    ofDisableAlphaBlending();
    ofPopStyle();
    
    
    glPushMatrix();
    glMultMatrixf(mapping[0].warper.getMatrix().getPtr());
    {
        mapping[0].fbo.draw(0, 0);
    }
    glPopMatrix();
    
    glPushMatrix();
    glMultMatrixf(mapping[1].warper.getMatrix().getPtr());
    {
        mapping[1].fbo.draw(0, 0);
    }
    glPopMatrix();
    
    
    //----------
    // draw base window
    //----------
    switch (mode) {
        case DEBUG_MODE:
            drawInfoText();
#ifdef AUDIO_PROCESS
            drawAudioVisualize(260, 80, 300, 40);
#endif
            break;
            
        case CALIB_MODE:
            drawCalibrationView();
            gui.draw();
            break;
            
        case VISUAL_MODE:
            break;
    }
    
    // info text temp draw
    if (mode != DEBUG_MODE && bDrawInfoText) {
        drawInfoText();
    }
    
#ifdef REC_VIDEO
    mGrabImg.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
    mCvframe = toCv(mGrabImg);
    cvWriteFrame(mCvVideoWriter, &mCvframe);
#endif
}

void testApp::drawCanvas(const int canvasID)
{
    switch (mode) {
        case DEBUG_MODE:
//            mCanvas[canvasID].drawDebug();
            mCanvas[canvasID].drawEffects();
            mCanvas[canvasID].drawDebug();
            break;
            
        case CALIB_MODE:
            mCanvas[canvasID].drawWhiteBack(80);
            break;
            
        case VISUAL_MODE:
            switch (mScene) {
                case SCENE_01:
                    mCanvas[canvasID].drawEffects();
                    break;
                    
                case SCENE_02:
                    mCanvas[canvasID].drawEffects();
                    break;
                    
                case SCENE_03:
                    mCanvas[canvasID].drawEffects();
                    break;
            }
            // when change the scenes action
            if (tmpScene != mScene) {
                
                tmpScene = mScene;
            }
            break;
    }
}

void testApp::drawCalibrationView()
{
    ofPushStyle();
    ofEnableAlphaBlending();
    ofNoFill();
    ofSetLineWidth(1);
    
    //----------
    // ruler
    //----------
    ofSetColor(0, 255, 0, 80);
    for (int i=0; i < ofGetWidth(); i += 50){
        ofLine(i, 0, i, ofGetHeight());
    }
    for (int i=0; i < ofGetHeight(); i += 50){
        ofLine(0, i, ofGetWidth(), i);
    }
    ofSetColor(0, 255, 255);
    ofRect(1, 1, ofGetWidth()-1, ofGetHeight()-1);
    ofLine(0, ofGetHeight()/2, ofGetWidth(), ofGetHeight()/2);
    ofLine(ofGetWidth()/2, 0, ofGetWidth()/2, ofGetHeight());
    
    //----------
    // Mapping Rect Lines
    //----------
    ofSetColor(0, 0, 255, 255);
    ofSetLineWidth(1);
    ofLine(mapping[0].cornerPt[0], mapping[0].cornerPt[1]);
    ofLine(mapping[0].cornerPt[1], mapping[0].cornerPt[2]);
    ofLine(mapping[0].cornerPt[2], mapping[0].cornerPt[3]);
    ofLine(mapping[0].cornerPt[3], mapping[0].cornerPt[0]);
    ofLine(mapping[1].cornerPt[0], mapping[1].cornerPt[1]);
    ofLine(mapping[1].cornerPt[1], mapping[1].cornerPt[2]);
    ofLine(mapping[1].cornerPt[2], mapping[1].cornerPt[3]);
    ofLine(mapping[1].cornerPt[3], mapping[1].cornerPt[0]);
    
    //----------
    // Mapping Rect Corners
    //----------
    ofSetColor(255, 0, 0, 255);
    int size = 10;
    ofSetLineWidth(3);
    switch (mMapPtsMode) {
        case C01_TOP_LEFT    : ofCircle(mapping[0].cornerPt[0], size); break;
        case C01_TOP_RIGHT   : ofCircle(mapping[0].cornerPt[1], size); break;
        case C01_BOTTOM_LEFT : ofCircle(mapping[0].cornerPt[3], size); break;
        case C01_BOTTOM_RIGHT: ofCircle(mapping[0].cornerPt[2], size); break;
        case C01_CENTER      : og.drawSquareLines(mapping[0].cornerPt[0],
                                                  mapping[0].cornerPt[1],
                                                  mapping[0].cornerPt[2],
                                                  mapping[0].cornerPt[3]); break;
        case C02_TOP_LEFT    : ofCircle(mapping[1].cornerPt[0], size); break;
        case C02_TOP_RIGHT   : ofCircle(mapping[1].cornerPt[1], size); break;
        case C02_BOTTOM_LEFT : ofCircle(mapping[1].cornerPt[3], size); break;
        case C02_BOTTOM_RIGHT: ofCircle(mapping[1].cornerPt[2], size); break;
        case C02_CENTER      : og.drawSquareLines(mapping[1].cornerPt[0],
                                                  mapping[1].cornerPt[1],
                                                  mapping[1].cornerPt[2],
                                                  mapping[1].cornerPt[3]); break;
        default: break;
    }
//    ofCircle(mapping[0].cornerPt[0], 10);
//    ofCircle(mapping[0].cornerPt[1], 10);
//    ofCircle(mapping[0].cornerPt[2], 10);
//    ofCircle(mapping[0].cornerPt[3], 10);
//    ofCircle(mapping[1].cornerPt[0], 10);
//    ofCircle(mapping[1].cornerPt[1], 10);
//    ofCircle(mapping[1].cornerPt[2], 10);
//    ofCircle(mapping[1].cornerPt[3], 10);

    
    ofDisableAlphaBlending();
    ofPopStyle();
}

void testApp::drawAudioVisualize(const int x, const int y, const int w, const int h)
{
    // draw the left channel:
	ofPushStyle();
    ofPushMatrix();
    ofTranslate(x, y, 0);
    
    ofSetColor(225);
    ofDrawBitmapString("Channel 1", 4, 18);
    
    ofSetColor(58, 245, 135);
    ofSetLineWidth(1);
    
    ofBeginShape();
    for (int i = 0; i < mSampleData.size(); i++){
        float x =  ofMap(i, 0, mSampleData.size(), 0, w, true);
        ofVertex(x, h/2 -mSampleData[i]*180.0f);
    }
    ofEndShape(false);
    
    ofPopMatrix();
	ofPopStyle();
}

//--------------------------------------------------------------
void testApp::exit()
{
    // save settings
    gui.saveSettings();
    mappingXml.saveFile(MAP_CALIB_XML);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
            
        //----- 矢印key -----
        case OF_KEY_LEFT:
            if (mode == CALIB_MODE) {
                setMappingCorners(-1, 0);
            } else {
                mCanvas[0].minusBlobNum();
                mCanvas[1].minusBlobNum();
            }
            break;
            
        case OF_KEY_RIGHT:
            if (mode == CALIB_MODE) {
                setMappingCorners(1, 0);
            } else {
                mCanvas[0].plusBlobNum();
                mCanvas[1].plusBlobNum();
            }
            break;
            
        case OF_KEY_UP:
            if (mode == CALIB_MODE) {
                setMappingCorners(0, -1);
            }
            break;
            
        case OF_KEY_DOWN:
            if (mode == CALIB_MODE) {
                setMappingCorners(0, 1);
            }
            break;
            
        //target corner point
        case 'm':
            mMapPtsMode++;
            if (mMapPtsMode == mappingPtsModeNum) {
                mMapPtsMode = (mappingPtsMode)0;
            }
            break;
            
        //-------------------
        case '_':
            switch (mode) {
                case DEBUG_MODE:
                    mode = CALIB_MODE;
                    break;
                case CALIB_MODE:
                    mode = VISUAL_MODE;
                    break;
                case VISUAL_MODE:
                    mode = DEBUG_MODE;
                    break;
            }
            break;

        case 'f':
            ofToggleFullscreen();
            break;
            
        case ' ':
            mode = VISUAL_MODE;
            ofSetFullscreen(true);
            break;
            
        //--------------------
#ifndef HONBAN
        //test
        case 'd':
//            mCanvas[1].setMapDetectColor(1);
//            mCanvas[1].setMapDownSample(32, 32);
//            mCanvas[1].setMapHexText(ofPoint(0, 0));
//            if (!mCanvas[0].mMapReturnScanLine.lines.empty()) {
//                mCanvas[1].makeScanHisto(mCanvas[0].mMapReturnScanLine.lines[0].posY, 4, 1, 3000);
//            }
            mCanvas[1].makeScanSpectrum(ofRandom(mCanvas[1].getCanvasHeight()), 4, 1, 5000);
            break;
        case 'D':
//            mCanvas[1].setMapDetectColor(0);
//            mCanvas[1].setMapDownSample(8, 8);
//            mCanvas[0].setMapReturnScanLine(3, 2);
            mCanvas[1].makeScanHisto(10, 1, 2000);
            break;
            
        case 'c':
            mCanvas[0].setMap();
            mCanvas[1].setMap();
            break;
            
        case 's':
//            mCanvas[1].makeScanPixels(2, 50);
            mCanvas[1].makeScanHisto(4, 1, 6000);
            break;
            
        case 'x':
            mCanvas[0].exportEachDitherNoiseWaveData();
            break;
            
        case 'I':
            bDrawInfoText = !bDrawInfoText;
            break;
#endif
            
        //----- 1~9 -----
        case '1': mScene = SCENE_01; break;
        case '2': mScene = SCENE_02; break;
        case '3': mScene = SCENE_03; break;
        case '4': break;
        case '5': break;
        case '6': break;
        case '7': break;
        case '8': break;
        case '9': break;
        
        //----- Shift 1~9 -----
        case '!': bFunction[0] = !bFunction[0]; break;
        case '"': bFunction[1] = !bFunction[1]; break;
        case '#': bFunction[2] = !bFunction[2]; break;
        case '$': bFunction[3] = !bFunction[3]; break;
        case '%': bFunction[4] = !bFunction[4]; break;
        case '&': bFunction[5] = !bFunction[5]; break;
        case '\'':bFunction[6] = !bFunction[6]; break;
        case '(': bFunction[7] = !bFunction[7]; break;
        case ')': bFunction[8] = !bFunction[8]; break;
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    gui.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    gui.mousePressed(x, y, button);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    gui.mouseReleased();
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 
}

void testApp::setMappingCorners(const int x, const int y)
{
    switch (mMapPtsMode) {
        case C01_TOP_LEFT:
            mapping[0].cornerPt[0].x += x;
            mapping[0].cornerPt[0].y += y;
            break;
            
        case C01_TOP_RIGHT:
            mapping[0].cornerPt[1].x += x;
            mapping[0].cornerPt[1].y += y;
            break;
            
        case C01_BOTTOM_LEFT:
            mapping[0].cornerPt[3].x += x;
            mapping[0].cornerPt[3].y += y;
            break;
            
        case C01_BOTTOM_RIGHT:
            mapping[0].cornerPt[2].x += x;
            mapping[0].cornerPt[2].y += y;
            break;
            
        case C01_CENTER:
            mapping[0].cornerPt[0].x += x;
            mapping[0].cornerPt[0].y += y;
            mapping[0].cornerPt[1].x += x;
            mapping[0].cornerPt[1].y += y;
            mapping[0].cornerPt[2].x += x;
            mapping[0].cornerPt[2].y += y;
            mapping[0].cornerPt[3].x += x;
            mapping[0].cornerPt[3].y += y;
            break;
            
        case C02_TOP_LEFT:
            mapping[1].cornerPt[0].x += x;
            mapping[1].cornerPt[0].y += y;
            break;
            
        case C02_TOP_RIGHT:
            mapping[1].cornerPt[1].x += x;
            mapping[1].cornerPt[1].y += y;
            break;
        
        case C02_BOTTOM_LEFT:
            mapping[1].cornerPt[3].x += x;
            mapping[1].cornerPt[3].y += y;
            break;
            
        case C02_BOTTOM_RIGHT:
            mapping[1].cornerPt[2].x += x;
            mapping[1].cornerPt[2].y += y;
            break;
            
        case C02_CENTER:
            mapping[1].cornerPt[0].x += x;
            mapping[1].cornerPt[0].y += y;
            mapping[1].cornerPt[1].x += x;
            mapping[1].cornerPt[1].y += y;
            mapping[1].cornerPt[2].x += x;
            mapping[1].cornerPt[2].y += y;
            mapping[1].cornerPt[3].x += x;
            mapping[1].cornerPt[3].y += y;
            break;
            
        default: break;
    }
    mapping[0].setCorners();
    mapping[1].setCorners();
    
    //----------
    // set xml
    //----------
    mappingXml.setValue("map_cnv0_c0_x", mapping[0].cornerPt[0].x);
    mappingXml.setValue("map_cnv0_c0_y", mapping[0].cornerPt[0].y);
    mappingXml.setValue("map_cnv0_c1_x", mapping[0].cornerPt[1].x);
    mappingXml.setValue("map_cnv0_c1_y", mapping[0].cornerPt[1].y);
    mappingXml.setValue("map_cnv0_c2_x", mapping[0].cornerPt[2].x);
    mappingXml.setValue("map_cnv0_c2_y", mapping[0].cornerPt[2].y);
    mappingXml.setValue("map_cnv0_c3_x", mapping[0].cornerPt[3].x);
    mappingXml.setValue("map_cnv0_c3_y", mapping[0].cornerPt[3].y);
    mappingXml.setValue("map_cnv1_c0_x", mapping[1].cornerPt[0].x);
    mappingXml.setValue("map_cnv1_c0_y", mapping[1].cornerPt[0].y);
    mappingXml.setValue("map_cnv1_c1_x", mapping[1].cornerPt[1].x);
    mappingXml.setValue("map_cnv1_c1_y", mapping[1].cornerPt[1].y);
    mappingXml.setValue("map_cnv1_c2_x", mapping[1].cornerPt[2].x);
    mappingXml.setValue("map_cnv1_c2_y", mapping[1].cornerPt[2].y);
    mappingXml.setValue("map_cnv1_c3_x", mapping[1].cornerPt[3].x);
    mappingXml.setValue("map_cnv1_c3_y", mapping[1].cornerPt[3].y);
}

//////////////////////////////////////////////////////////////////
// Audio Out
//////////////////////////////////////////////////////////////////
void testApp::audioOut(float * output, int bufferSize, int nChannels){
#ifdef AUDIO_PROCESS
    for (int i = 0; i < bufferSize; i++) {
        float sample = 0;        
        sample *= gui.getValueF(GUI_MASTER_VOLUME);
        sample = ofClamp(sample, -1, 1);
        mSampleData[i*nChannels] = output[i*nChannels] = sample;
    }
#endif
}
