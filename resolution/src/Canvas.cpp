#include "Canvas.h"

using namespace ofxCv;
using namespace cv;
using namespace myMess;

ofImage tImg; //!<原画像表示用
ofImage tEdgeImg; //!<エッジ画像表示用
ofImage tMapDstImg;
int lowFpsCount = 0;

ofPixels detectBlob(ofImage& srcImg, const unsigned int blobNum);
ofPixels detectBlobAlph(ofImage& blobImg);
ofColor detectBlobColor(ofPixels& blobPix, ofPixels& rgbPix);

//--------------------------------------------------------------

Canvas::Canvas():
mCanvasID(-1),
mCanvasWidth(0),
mCanvasHeight(0),
mBlobNum(0),
mMaxBlob(0),
mTmpBlobNum(-1),
mBlobsCenter(ofPoint(ofGetWidth()/2, ofGetHeight()/2)),
mVGazePos(ofPoint(0,0)),
mResoNumX(0),
mResoNumY(0),
mDithNoiseCounter(0),
//==== Visual Effects =====
bDrawWhite(false),
bDrawOrgImg(false),
bDrawLabelImg(false),
mBackColor(ofColor(0, 0, 0, 0)),
mFrontColor(ofColor(0, 0, 0, 0)),
mCurrentSubImg(NULL)
{
    for(int i = 0; i < bFunction.size(); i++)bFunction[i] = true;
//    mFont.loadFont("Indieutka_Pixel8.ttf", 6);
    for (int i = 0; i < bDrawSubImg.size(); i++) bDrawSubImg = false;
}

//--------------------------------------------------------------

Canvas::~Canvas()
{
    mMyBlobs.clear();
    vector<myBlob>(mMyBlobs).swap(mMyBlobs);
}

//-------------------------------------------------------------- setter
void Canvas::setOscSender(const string& host, const int poat)
{
    sender.setup(host, poat);
}

void Canvas::setCanvasSize(const int width, const int height)
{
    mCanvasWidth = width;
    mCanvasHeight = height;
}

/** ２枚目の画像を読み込む
 @param path 画像のパス
 */
void Canvas::setColorImage(const string& path, const bool resetCanvasSize)
{
    if (!path.empty()) {
        if (mOrgImg.isAllocated()) mOrgImg.clear();
        if (mOrgImg.loadImage(path)) {
            //labelImageがある場合、各要素の平均色を取得
            assert(mLabelImg.isAllocated());
            if (mLabelImg.isAllocated()) {
                for (int i=0; i < mMyBlobs.size(); i++) {
                    mMyBlobs[i].color.set( ofColor(detectBlobColor(mMyBlobs[i].blobImg.getPixelsRef(), mOrgImg.getPixelsRef())) );
                    cout << "set blob color: MyBlob[" << i << "]" << endl;
                }
            }
            if (resetCanvasSize) {
                setCanvasSize(mOrgImg.width, mOrgImg.height);
            }
            //TODO: init read image params
            mImageParams = calcImageParameters(mOrgImg);
            mCurrentSubImg = &mOrgImg;
            return;
        } else {
            cout << "faild load color image: " << path << endl;
        }
    }
}

/** 
 ラベリング画像を読み込んですべてのblobを計算する
 @param 
 */
void Canvas::setLabelingImageAndCreateBlobs(const string& path, const bool resetCanvasSize)
{
    assert(mLabelImg.loadImage(path));
    if (mLabelImg.getPixelsRef().getNumChannels() != 1)
        cout << "[WARNING] channel number is not 1" << endl;
    if (resetCanvasSize) setCanvasSize(mLabelImg.width, mLabelImg.height);
    
    //最大画素値を取得
    unsigned int ch = mLabelImg.getPixelsRef().getNumChannels();
    for (int i=0; i < mLabelImg.width * mLabelImg.height; i++) {
        if (mMaxBlob < mLabelImg.getPixelsRef()[i*ch]) {
            mMaxBlob = mLabelImg.getPixelsRef()[i*ch];
        }
    }
    cout << "mMaxBlob: " << mMaxBlob << endl;
    //最大値が255以上だった場合はエラー
    if (mMaxBlob >= 255) {
        ofSystemAlertDialog("[ERROR] blobs num is over 255");
        ofExit();
    }
    
    //==========
    // 全blobとその内容を取得して配列に格納
    //==========
    ofxCvGrayscaleImage cvGrayImg;
    ofxCvContourFinder contFind;
    for (int i = 0; i <= mMaxBlob; i++) {
        myBlob tmpBlob;
        //IDをセット
        tmpBlob.ID = i;
        //blob画像をセット
        tmpBlob.blobImg.setFromPixels(detectBlob(mLabelImg, i));
        //アルファチャンネル付きblob画像をセット
//        tmpBlob.blobImgAlph.setFromPixels(detectBlobAlph(tmpBlob.blobImg));
        //cvBlobをセット
        if (cvGrayImg.bAllocated) {
            cvGrayImg.clear();
            cvGrayImg.allocate(mLabelImg.width, mLabelImg.height);
        }
        cvGrayImg.setFromPixels(tmpBlob.blobImg.getPixelsRef());
        int bnum = contFind.findContours(cvGrayImg, 5, mLabelImg.width * mLabelImg.height, 1, false);
        if (bnum > 1) cout << "[WORNING] contours find some blobs | blob ID: " << tmpBlob.ID << endl;
        tmpBlob.cvBlob = contFind.blobs[0];
        //ofColorの初期値をセット
        tmpBlob.color.set(0, 0, 0);
        //中心からの距離の初期値をセット
        tmpBlob.distToCentPos = 0.0f;
        //配列に格納
        mMyBlobs.push_back(tmpBlob);
        cout << "create blob " << i << endl;
    }
    
    //----------
    //【全体の重心を計算】
    // 中心からの距離と大きさの両方に影響しあうように、
    // a=前塊の大きさを画素値とした重心
    // b=中心から最も近い距離にある塊の重心
    // (a-b)/2 の点にある座標を計算する
    //----------
    ofxCvFloatImage sizeImg;
    ofPoint centPt;
    ofPoint areaPt;
    
    sizeImg.allocate(mLabelImg.width, mLabelImg.height);
    sizeImg.set(0);
    // 全塊の重心からさらに重心を計算（背景blobを除外）
    ofPoint sumCent = ofPoint(0, 0);
    for (int i=1; i < mMyBlobs.size(); i++) {
        sumCent.x += mMyBlobs[i].cvBlob.centroid.x;
        sumCent.y += mMyBlobs[i].cvBlob.centroid.y;
    }
    centPt = ofPoint(sumCent.x/mMyBlobs.size(), sumCent.y/mMyBlobs.size());
    
    //（以下保留）
    // エリアを0~1で正規化して32f画像を作成
    //    float maxArea = 0;
    //    for (int i=0; i < areas.size(); i++)
    //        if (areas[i] > maxArea) maxArea = areas[i];
    //    for (int i=0; i < areas.size(); i++)
    //        areas[i] = areas[i] / maxArea;
    //    for (int i=0; i < areas.size(); i++) {
    //        ofxCvFloatImage tmpFloatImg;
    //        tmpFloatImg.allocate(mLabelImg.width, mLabelImg.height);
    //        tmpFloatImg.setFromPixels(detectBlob(mLabelImg, i+1, areas[i]));
    //        sizeImg += tmpFloatImg;
    //    }
    // 全画素の総数を計算
    //    float W = 0.0f;
    //    float sumX = 0.0f;
    //    float xumY = 0.0f;
    //    for (int i=0; i < sizeImg.width*sizeImg.height; i++)
    //        W += sizeImg.getPixelsAsFloats()[i];
    //    for (int y=0; y < sizeImg.height; y++) {
    //        for (int x=0; x < sizeImg.width; x++) {
    //        }
    //    }
    //（とりあえず最大サイズの塊の重心を
    float maxArea = 0.0f;
    int maxAreaId = 0;
    for (int i=1; i < mMyBlobs.size(); i++)
        if (maxArea < mMyBlobs[i].cvBlob.area) {
            maxAreaId = i;
            maxArea = mMyBlobs[i].cvBlob.area;
        }
    areaPt.set(mMyBlobs[maxAreaId].cvBlob.centroid);
    
    // ２つの重心の平均値を計算
    mBlobsCenter.set((centPt.x+areaPt.x)/2, (centPt.y+areaPt.y)/2);
}

/** エフェクト用画像を追加する
 @return セット後の画像配列のサイズ
 */
int Canvas::addSubImage(const string &path)
{
    ofImage img;
    assert(img.loadImage(path));
    mSubImg.push_back(img);
    assert(mSubImg.size() <= bDrawSubImg.size());
    return mSubImg.size();
}
 
void Canvas::setBlobNum(const int num)
{
    if (num >= 0 && num <= mMaxBlob) {
        mBlobNum = num;
    } else {
        cout << "over size blob num: " << num << endl;
    }
}

void Canvas::plusBlobNum()
{
    mBlobNum++;
    if (mBlobNum > mMaxBlob) mBlobNum = 0;
}

void Canvas::minusBlobNum()
{
    mBlobNum--;
    if (mBlobNum < 0) mBlobNum = mMaxBlob;
}

//--------------------------------------------------------------
/**
 マルコフモデルを描画
 */
void Canvas::drawMarkov()
{
    float W = 1024;
    float H = 768;
    int size = 15;
        
    //----------
    // 先に全blobの座標点を計算
    //----------
    vector<ofPoint> pts;
    for (int i=0; i < mMyBlobs.size(); i++) {
        if (i == 0) {
            pts.push_back(ofPoint(W/2, H/2));
        } else {
            float x = sin(ofDegToRad(((i-1) * (359 / mMaxBlob)))) * (H/3);
            float y = cos(ofDegToRad(((i-1) * (359 / mMaxBlob)))) * -(H/3);
            pts.push_back(ofPoint(W/2 + x, H/2 + y));
        }
    }
    
    //----------
    // 線を描画
    //----------
//    for (int j=0; j < pts.size(); j++) {
//        if (j != mBlobNum) {
//            float p = 0;
//            for (int k=0; k < mMyBlobs[mBlobNum].moveProbs.size(); k++) {
//                if (k == j) {
//                    p = mMyBlobs[mBlobNum].moveProbs[k];
//                }
//            }
//            ofSetColor(180, 180, 230, p*255);
//            p = ofMap(p, 0.0, 1.0, 0, 30);
//            if (p > 0.0) {
//                ofSetLineWidth(p);
//                ofLine(pts[mBlobNum], pts[j]);
//            }
//        }
//    }
    
    //----------
    // ノードを描画
    //----------
    for (int i=0; i < pts.size(); i++) {
        //もしソースイメージが読まれている場合はblobカラーを描画
        if (mOrgImg.bAllocated()) {
            ofFill();
            ofSetColor(mMyBlobs[i].color);
            size = 10;
            ofCircle(pts[i], size);
        }
        
        ofNoFill();
        ofSetLineWidth(2);
        if (i == 0) {
            if (i == mBlobNum) {
                size = 15;
                ofSetColor(255, 0, 0);
            } else {
                size = 10;
                ofSetColor(0);
            }
            ofCircle(pts[0], size);
            ofSetColor(255);
            ofDrawBitmapString(ofToString(i), pts[i]);
            
        } else {
            
            if (i == mBlobNum) {
                size = 15;
                ofSetColor(255, 0, 0);
            } else {
                size =10;
                ofSetColor(255, 255, 255);
                ofCircle(pts[i], size);
                
            }
            ofCircle(pts[i], size);
            ofSetColor(0);
            ofDrawBitmapString(ofToString(i), pts[i]);
        }
    }
}

void Canvas::drawDebug()
{
    /*
    if (bFunction[0]) {
        //--------------------
        //blobを描画
        //--------------------
        ofSetColor(255, 127);
        mMyBlobs[mBlobNum].blobImg.draw(0, 0);
        mMyBlobs[mBlobNum].cvBlob.draw();
    }
    
    //----------
    //擬似視線を描画
    //----------
    if (bFunction[8]) drawVirtualGazePoint();
    
    if (bFunction[1]) {
        //blobの重心の重心座標を描画
        ofSetColor(80);
        ofLine(mBlobsCenter.x-15, mBlobsCenter.y, mBlobsCenter.x+15, mBlobsCenter.y);
        ofLine(mBlobsCenter.x, mBlobsCenter.y-15, mBlobsCenter.x, mBlobsCenter.y+15);
        
        //中心からblob重心までの線を描画
        ofSetColor(255, 0, 0);
        ofPoint pos = mMyBlobs[mBlobNum].cvBlob.centroid;
        ofLine(pos.x-10, pos.y-10, pos.x+10, pos.y+10);
        ofLine(pos.x+10, pos.y-10, pos.x-10, pos.y+10);
        ofSetColor(255,255,255, 60);
        ofLine(pos.x, pos.y, mBlobsCenter.x, mBlobsCenter.y);
        
        //他のblobへの移動確率を描画
        for (int i=0; i < mMyBlobs.size(); i++) {
            if (i != mBlobNum) {
                float p = 0;
                for (int j=0; j < mMyBlobs[mBlobNum].moveProbs.size(); j++) {
                    if (j == i) {
                        p = mMyBlobs[mBlobNum].moveProbs[j];
                    }
                }
                ofSetColor(180, 180, 230, p*255);
                p = ofMap(p, 0.0, 1.0, 0, 50);
                if (p > 0.0) {
                    ofSetLineWidth(p);
                    ofLine(mMyBlobs[mBlobNum].cvBlob.centroid, mMyBlobs[i].cvBlob.centroid);
                }
            }
        }
        
        
        //ルーラーを描画
        ofSetColor(26);
        ofSetLineWidth(0.5);
        ofLine(mLabelImg.width/2, 0, mLabelImg.width/2, mLabelImg.height);
        ofLine(0, mLabelImg.height/2, mLabelImg.width, mLabelImg.height/2);
        
        //平均カラーを描画
        if (mOrgImg.isAllocated()) {
            ofSetColor(255);
            ofNoFill();
            ofSetLineWidth(2);
            ofCircle(ofGetWidth()-20, 20, 11);
            ofSetColor(mMyBlobs[mBlobNum].color);
            ofFill();
            ofCircle(ofGetWidth()-20, 20, 10);
        }
        
        //ルーラーを描画
        ofSetLineWidth(0.5);
        ofSetColor(200);
        for (int i=0; i < mLabelImg.width; i += 10){
            if (i%100 == 0)
                ofLine(i, 0, i, 15);
            else
                ofLine(i, 0, i, 10);
        }
        for (int i=0; i < mLabelImg.height; i += 10){
            if (i%100 == 0)
                ofLine(0, i, 15, i);
            else
                ofLine(0, i, 10, i);
        }
        
        //十字のルーラーを描画
        ofSetColor(120, 150, 120, 60);
        ofLine(mMyBlobs[mBlobNum].cvBlob.centroid.x, 0, mMyBlobs[mBlobNum].cvBlob.centroid.x, mLabelImg.height);
        ofLine(0, mMyBlobs[mBlobNum].cvBlob.centroid.y, mLabelImg.width, mMyBlobs[mBlobNum].cvBlob.centroid.y);
        ofSetColor(255,255,255,180);
        ofDrawBitmapString(ofToString(mMyBlobs[mBlobNum].cvBlob.centroid.x, 2), mMyBlobs[mBlobNum].cvBlob.centroid.x+10, 25);
        ofDrawBitmapString(ofToString(mMyBlobs[mBlobNum].cvBlob.centroid.y, 2), 15, mMyBlobs[mBlobNum].cvBlob.centroid.y+10);
        
        //Dither
        ofSetLineWidth(0.5);
        ofSetColor(120, 150, 120, 127);
        if (mResoNumX > 0 && mResoNumY > 0) {
            for (int i = 0; i < mLabelImg.width; i += mResoNumX) {
                ofLine(i, 0, i, mLabelImg.height);
            }
            for (int i = 0; i < mLabelImg.height; i += mResoNumY) {
                ofLine(0, i, mLabelImg.width, i);
            }
        }
    }
    */
    
    //TODO: test
    ofSetColor(255, 0, 0, 80);
    for (int i = 0; i < mImageParams.histR.size(); i++) {
        ofLine(i, mCanvasHeight, i, mCanvasHeight-(mImageParams.histR[i] * 200));
    }
    ofSetColor(0, 255, 0, 80);
    for (int i = 0; i < mImageParams.histG.size(); i++) {
        ofLine(i, mCanvasHeight, i, mCanvasHeight-(mImageParams.histG[i] * 200));
    }
    ofSetColor(0, 0, 255, 80);
    for (int i = 0; i < mImageParams.histB.size(); i++) {
        ofLine(i, mCanvasHeight, i, mCanvasHeight-(mImageParams.histB[i] * 200));
    }
}

void Canvas::drawInfoText(const int x, const int y)
{
    //----------
    //情報（テキスト）を描画
    //----------
    stringstream ss;
    getInfoText(ss);
    ofSetColor(255);
    ofDrawBitmapString(ss.str(), x, y);
}

void Canvas::getInfoText(stringstream& s)
{
    s << "===== Canvas: " << mCanvasID << " =====" << endl;
    s << "blob          : " << mBlobNum << endl;
    s << "max           : " << mMaxBlob << endl;
    s << "blob centor x : " << mMyBlobs[mBlobNum].cvBlob.centroid.x << endl;
    s << "blob centor y : " << mMyBlobs[mBlobNum].cvBlob.centroid.y << endl;
    s << "blob area     : " << mMyBlobs[mBlobNum].cvBlob.area << endl;
    s << "blob length   : " << mMyBlobs[mBlobNum].cvBlob.length << endl;
    s << "center dist   : " << mMyBlobs[mBlobNum].cvBlob.centroid.distance(mBlobsCenter)
    << endl;
    if (mOrgImg.isAllocated()) {
        s << "blob mean R: " << (float)mMyBlobs[mBlobNum].color.r << endl;
        s << "blob mean G: " << (float)mMyBlobs[mBlobNum].color.g << endl;
        s << "blob mean B: " << (float)mMyBlobs[mBlobNum].color.b << endl;
    }
    s << "resolution X: " << mResoNumX << " Y: " << mResoNumY << endl;
    s << "dith noise size X: " << mDithNoiseX.size() << " Y: " << mDithNoiseY.size() << endl;
    s << "scan lines: " << mScanLines.size() << endl;
    s << "scan edges: " << mScanEdges.size() << endl;
    s << "scan pixel: " << mScanPixels.size() << endl;
    s << "scan histo: " << mScanHistos.size() << endl;
}

void Canvas::drawOrgImage(){
    if (mOrgImg.isAllocated()) {
        ofSetColor(255);
        mOrgImg.draw(0, 0);
    }
}

void Canvas::drawWhiteBack(const unsigned char brightness)
{
    ofSetColor(brightness);
    ofRect(0, 0, mCanvasWidth, mCanvasHeight);
}

void Canvas::drawColorBack(const ofColor color)
{
    ofSetColor(color);
    ofRect(0, 0, mCanvasWidth, mCanvasHeight);
}

void Canvas::testDraw()
{
    
    //TODO: dith wave
    vector<float> tmpDithX = getDitherNoiseWavePts(mMyBlobs[mBlobNum].blobImg, mResoNumX, true);
    vector<float> tmpDithY = getDitherNoiseWavePts(mMyBlobs[mBlobNum].blobImg, mResoNumY, false);
    ofSetColor(255, 0, 0);
    float tmp = 0;
    int geta = ofGetHeight()/2;
    for (int i = 0; i < tmpDithX.size(); i++) {
        ofLine(i, tmp + geta, i+1, tmpDithX[i] + geta);
        tmp = tmpDithX[i];
    }
    ofSetColor(0, 0, 255);
    tmp = 0;
    for (int i = 0; i < tmpDithY.size(); i++) {
        ofLine(i, tmp + geta, i+1, tmpDithY[i] + geta);
        tmp = tmpDithY[i];
    }
    ofSetColor(0, 255, 0);
    tmp = 0;
    for (int i = 0; i < mDithNoiseXComp.size(); i++) {
        ofLine(i, (tmp*100) + geta, i+1, (mDithNoiseXComp[i]*100) + geta);
        tmp = mDithNoiseXComp[i];
    }
    
    
}


//---------------------------------------------------

/** @brief 入力画像から１つのblobを抽出します
 カラー画像として入力してもそのうちの１ch（Rチャンネル）の画素のみを取得してグレースケール画像とします
 @param srcImg 入力画像（ラベリング画像）
 @param blobNum どのIDのblobを取得するか
 @return 取得したblobのピクセル
 */
ofPixels detectBlob(ofImage& srcImg, const unsigned int blobNum)
{
    ofPixels dstPix;
    dstPix.allocate(srcImg.width, srcImg.height, 1);
    
    unsigned char* px = new unsigned char[srcImg.width * srcImg.height]();
    unsigned int ch = srcImg.getPixelsRef().getNumChannels();
    for (int i=0; i < srcImg.width * srcImg.height; i++) {
        if (srcImg.getPixels()[i*ch] == blobNum) {
            px[i] = 255;
        } else {
            px[i] = 0;
        }
    }
    dstPix.setFromPixels(px, srcImg.width, srcImg.height, 1);
    free(px);
    return dstPix;
}

/** @brief blob画像から0の画素をアルファチャンネル0に置き換えます
 @param blobImg 入力画像
 @return 取得したアルファチャンネル付きblob画像ピクセル
 */
ofPixels detectBlobAlph(ofImage& blobImg)
{
    ofPixels dstPix;
    dstPix.allocate(blobImg.width, blobImg.height, OF_IMAGE_COLOR_ALPHA);
    
    unsigned char* px = new unsigned char[blobImg.width * blobImg.height * 4]();
    unsigned int ch = blobImg.getPixelsRef().getNumChannels();
    for (int i=0; i < blobImg.width * blobImg.height; i++) {
        if (blobImg.getPixels()[i] == 0) {
            px[i*4+0] = 0;
            px[i*4+1] = 0;
            px[i*4+2] = 0;
            px[i*4+3] = 0;
        } else {
            px[i*4+0] = 255;
            px[i*4+1] = 255;
            px[i*4+2] = 255;
            px[i*4+3] = 255;
        }
    }
    dstPix.setFromPixels(px, blobImg.width, blobImg.height, OF_IMAGE_COLOR_ALPHA);
    free(px);
    return dstPix;
}


/** @brief blobからRGB平均色を取得します
 @param blobPix blob画像ピクセル
 @param rgbPix 入力画像の原画像（色の取得先となる画像）
 @return ofColor
 */
ofColor detectBlobColor(ofPixels& blobPix, ofPixels& rgbPix)
{
    ofColor dst;
    unsigned int ch = rgbPix.getNumChannels();
    if (ch >= 3) {
        vector<unsigned char> tmpRs;
        vector<unsigned char> tmpGs;
        vector<unsigned char> tmpBs;
        
        for (int i=0; i < blobPix.getWidth() * blobPix.getHeight(); i++) {
            if (blobPix.getPixels()[i] > 0) {
                tmpRs.push_back(rgbPix.getPixels()[i*ch+0]);
                tmpGs.push_back(rgbPix.getPixels()[i*ch+1]);
                tmpBs.push_back(rgbPix.getPixels()[i*ch+2]);
            }
        }
        if (tmpRs.empty() || tmpGs.empty() || tmpBs.empty()) {
            return dst;
        }
        int tmpSumR = 0;
        int tmpSumG = 0;
        int tmpSumB = 0;
        for (int i=0; i < tmpRs.size(); i++) tmpSumR += tmpRs[i];
        for (int i=0; i < tmpGs.size(); i++) tmpSumG += tmpGs[i];
        for (int i=0; i < tmpBs.size(); i++) tmpSumB += tmpBs[i];
        dst.set(tmpSumR / tmpRs.size(), tmpSumG / tmpGs.size(), tmpSumB / tmpBs.size());
    }
    return dst;
}

/** @brief 擬似注視点を描画 */
void Canvas::drawVirtualGazePoint()
{
    float tmpX = (mMyBlobs[mBlobNum].cvBlob.centroid.x - mVGazePos.x) / 3;
    float tmpY = (mMyBlobs[mBlobNum].cvBlob.centroid.y - mVGazePos.y) / 3;
    //固視微動の再現のため、ちょっとだけランダムに座標ずらす
    tmpX += ofRandom(-5, 5);
    tmpY += ofRandom(-5, 5);
    mVGazePos.x += tmpX;
    mVGazePos.y += tmpY;
    ofPushStyle();
    ofFill();
    ofSetColor(0, 255, 0);
    ofCircle(mVGazePos, 8);
    ofPopStyle();
}

/**
 指定したXYの格子で量子化誤差ノイズ波形を返す
 */
vector<float> Canvas::getDitherNoiseWavePts(ofImage& blobImg, int reso, bool isX)
{
    // check value
    if (isX) {
        if (reso > mLabelImg.width) reso = mLabelImg.width;
        if (reso > blobImg.getWidth()) reso = blobImg.getWidth();
    } else {
        if (reso > mLabelImg.height) reso = mLabelImg.height;
        if (reso > blobImg.getHeight()) reso = blobImg.getHeight();
    }
    
    vector<float> dst;
    vector<ofPoint> cntPts = og.getContourPoints(blobImg.getPixelsRef());
    vector<ofPoint>::iterator it = cntPts.begin();
    while (it != cntPts.end()) {
        if (isX) {
            int sample = (*it).x / reso;
            sample *= reso;
            float dstSmp = (*it).x - sample;
            dst.push_back(dstSmp);
        } else {
            int sample = (*it).y / reso;
            sample = (sample * reso) + reso;
            float dstSmp = (*it).y - sample;
            dst.push_back(dstSmp);
        }
        it++;
    }
    
    return dst;
}

/**
 量子化ノイズをテキストに書き出す(coll format)
 */
void Canvas::exportDitherNoiseWaveData(vector<vector<float> > wavePts, const string& path)
{
    if (!path.empty()) {
        std::ofstream log;
        log.open(path.c_str());
        log << "resoX " << mResoNumX << ";" << endl;
        log << "resoY " << mResoNumY << ";" << endl;
        log << "blob_num " << wavePts.size() << ";" << endl;
        for (int i = 0; i < wavePts.size(); i++) {
            log << "blob " << i << ";" << endl;
            log << "size " << wavePts[i].size() << ";" << endl;
            vector<float>::iterator maxIt = std::max_element(wavePts[i].begin(), wavePts[i].end());
            log << "max " << (*maxIt) << ";" << endl;
            for (int j = 0; j < wavePts[i].size(); j++) {
                log << "data " << wavePts[i][j] << ";" << endl;
            }
        }
        log << "[end]";
        log.close();
    }
}

/**
 量子化ノイズを2, 4, 8, 16, 32のpixelsizeに分けてテキストに書き出す
 */
void Canvas::exportEachDitherNoiseWaveData()
{
    ofFileDialogResult res = ofSystemSaveDialog("data", "save dither noise data");
    if (res.bSuccess) {
        system(("mkdir " + res.getPath()).c_str());
        for (int i = 2; i <= 32; i += i) {
            mResoNumX = i;
            mResoNumY = i;
            vector<vector<float> > tWaveX;
            vector<vector<float> > tWaveY;
            for (int j = 0; j < mMaxBlob; j++) {
                vector<float> tmpX = getDitherNoiseWavePts(mMyBlobs[j].blobImg, i, true);
                vector<float> tmpY = getDitherNoiseWavePts(mMyBlobs[j].blobImg, i, false);
                tWaveX.push_back(tmpX);
                tWaveY.push_back(tmpY);
            }
            exportDitherNoiseWaveData(tWaveX, res.getPath() + "/dith_" + ofToString(i) + "_x.txt");
            exportDitherNoiseWaveData(tWaveY, res.getPath() + "/dith_" + ofToString(i) + "_y.txt");
        }
    }
}

ImageParams Canvas::calcImageParameters(ofImage& img)
{
    cout << "Calc ImageParams..." << endl;
    if (!img.isAllocated() || img.getPixelsRef().getNumChannels() < 3) {
        cout << "[ERROR] faild calc image params" << endl;
        return;
    }
    
    ImageParams params;
    int imgSize = img.width * img.height;
    long long int sumR = 0;
    long long int sumG = 0;
    long long int sumB = 0;
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            sumR += img.getColor(x, y).r;
            sumR += img.getColor(x, y).g;
            sumR += img.getColor(x, y).b;
        }
    }
    params.avgColor.set((float)(sumR / (imgSize*255)), (float)(sumG / (imgSize*255)), (float)(sumB / (imgSize*255)));
    cout << "set average color" << endl;
    params.histR = og.getHistogram(img, 1, true);
    cout << "set histogram red" << endl;
    params.histG = og.getHistogram(img, 2, true);
    cout << "set histogram green" << endl;
    params.histB = og.getHistogram(img, 3, true);
    cout << "set histogram blue" << endl;
    
    return params;
}

//=============================================================
void Canvas::updateEffects(){
    //==========
    // Mappiong Effects update
    //==========
    // Mapping Down Sampling image
    //----------
    if (mMapDownSample.enable) {
    }
    
    //----------
    // Mapping Hex Text
    //----------
    if (mMapHexText.enable) {
    }
    
    //----------
    // Mapping Detect Color
    //----------
    if (mMapDetectColor.enable) {
        mMapDetectColor.pos += mMapDetectColor.speed;
        if (mMapDetectColor.pos > 255) mMapDetectColor.pos = 0;
    }
    
    //----------
    // Mapping Retrun Scan Line
    //----------
    if (mMapReturnScanLine.enable) {
        if (!mMapReturnScanLine.lines.empty()) {
            mMapReturnScanLine.lines[0].posY += (1/ofGetFrameRate()) * mMapReturnScanLine.lines[0].speed;
            if (mMapReturnScanLine.lines[0].posY < 0 ||
                mMapReturnScanLine.lines[0].posY > mCanvasHeight) {
                mMapReturnScanLine.lines[0].speed *= -1;
            }
        }
    }
    
    //==========
    // Scan Effect update
    //==========
    // Scan Line
    //----------
    if (!mScanLines.empty()) {
        vector<ScanLine>::iterator it = mScanLines.begin();
        while (it != mScanLines.end()) {
//            (*it).posY -= (*it).speed;
            it->posY -= (1/ofGetFrameRate()) * (*it).speed;
            
            if ((*it).posY + (*it).w < 0) {
                it = mScanLines.erase(it);
            } else {
                // tmp sender
                if (it->posY == mCanvasHeight/2) {
                    sendOsc(SCAN_LINE_GET_HALF, "get");
                }
                
                it++;
            }
        }
    }
    
    //---------
    // Scan Edge
    //---------
    if (mScanEdges.size()) {
        vector<ScanLine>::iterator it = mScanEdges.begin();
        while (it != mScanEdges.end()) {
            it->posY -= (1/ofGetFrameRate()) * (*it).speed;
            
            if ((*it).posY + (*it).w < 0) {
                it = mScanEdges.erase(it);
            } else {
                it++;
            }
        }
    }
    
    //---------
    // Scan Pixels
    //---------
    if (mScanPixels.size() && mOrgImg.isAllocated()) {
        vector<ScanLine>::iterator it = mScanPixels.begin();
        while (it != mScanPixels.end()) {
            it->posY -= (1/ofGetFrameRate()) * (*it).speed;
            
            if ((*it).posY + (*it).w < 0) {
                it = mScanPixels.erase(it);
            } else {
                it++;
            }
        }
    }
    
    //----------
    // Scan Histogram
    //----------
    if (mScanHistos.size() && mOrgImg.isAllocated()) {
        vector<ScanLineExtra>::iterator it = mScanHistos.begin();
        while (it != mScanHistos.end()) {
            if (it->posX <= mOrgImg.width) {
                //first : grow the line
                it->posX += (1/ofGetFrameRate()) * it->speed;
                it++;
            } else {
                //second : fade out
                it->life -= (1/ofGetFrameRate()) * 1000;
                if (it->life <= 0) {
                    it = mScanHistos.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    
    //----------
    // Scan Spectrum
    //----------
    if (mScanSpectrum.size() && mOrgImg.isAllocated()) {
        vector<ScanLineExtra>::iterator it = mScanSpectrum.begin();
        while (it != mScanSpectrum.end()) {
            if (it->posX <= mOrgImg.width) {
                //1 : grow the line
                it->posX += (1/ofGetFrameRate()) * it->speed;
                it++;
            } else if (it->secondScanPos < mOrgImg.width) {
                //2 : second scan
                it->secondScanPos += (1/ofGetFrameRate()) * it->secondScanSpeed;
                it++;
            } else {
                it->life -= (1/ofGetFrameRate()) * 1000;
                if (it->life <= 0) {
                    it = mScanSpectrum.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    
    //----------
    // Gaze Object
    //----------
    if (mGazeObject.size()) {
        vector<BlobObject>::iterator it = mGazeObject.begin();
        while (it != mGazeObject.end()) {
            it->size += it->decay;
            it->life -= it->decay;
            
            if (it->life <= 0) {
                it = mGazeObject.erase(it);
            } else {
                it++;
            }
        }
    }
    
    //panic reset
    if (ofGetFrameRate() < 10.0f) {
        lowFpsCount++;
        if (lowFpsCount > 10) {
            ClearAllEffects();
            lowFpsCount = 0;
        }
    } else {
        lowFpsCount = 0;
    };
}

//-----------------------------------------------------
void Canvas::drawEffects(const bool overDraw)
{
    ofEnableAlphaBlending();
    ofPushStyle();
    
    if (overDraw) drawColorBack(ofColor(0, 0, 0, 255));
    
    // front color mapping
    ofFill();
    ofSetColor(mBackColor);
    ofRect(0, 0, mCanvasWidth, mCanvasHeight);
    
    //----------
    // Draw Image
    //----------
    // Original Color Image
    if (bDrawOrgImg && mOrgImg.isAllocated()) {
        ofSetColor(255);
        mOrgImg.draw(0, 0);
        if (mCurrentSubImg != &mOrgImg)
            mCurrentSubImg = &mOrgImg;
    }
    // Labeling Image
    if (bDrawLabelImg && mLabelImg.isAllocated()) {
        ofSetColor(255);
        mLabelImg.draw(0, 0);
        if (mCurrentSubImg != &mLabelImg)
            mCurrentSubImg = &mLabelImg;
    }
    // Sub Image
    if (!mSubImg.empty()) {
        for (int i = 0; i < mSubImg.size(); i++) {
            if (bDrawSubImg[i] && mSubImg[i].isAllocated()) {
                ofSetColor(255);
                mSubImg[i].draw(0, 0);
                //test
                if (mCurrentSubImg != &mSubImg[i])
                    mCurrentSubImg = &mSubImg[i];
            }
        }
    }
    

    
    //----------
    // Draw Mapping Effects
    //----------
    if (mMapDownSample.enable) drawMapDownSampl();
    if (mMapHexText.enable) drawMapHexText();
    if (mMapDetectColor.enable) drawMapDetectColorBlob();
    if (mMapReturnScanLine.enable) drawMapReturnScanLine();
    if (mMapDetectBlobs.enable) drawMapDetectBlobs();
        
    //----------
    // Scan Line
    //----------
    if (mScanLines.size()) {
        ofFill();
        for (int i = 0; i < mScanLines.size(); i++) {
            for (int j = mScanLines[i].w; j > 0; j--) {
                int tmpY = mScanLines[i].posY - (-mScanLines[i].w + j);
                //上下にはみ出した分は描画しない
                if (tmpY < 0 || tmpY > mCanvasHeight) continue;
                ofSetColor(mScanLines[i].col, ofMap(j, 0, mScanLines[i].w, 0, 255));
                ofRect(0, tmpY, mCanvasWidth, 1);
            }
            ofSetColor(255, 255);
//            mFont.drawString(ofToString(mScanLines[i].posY), 5, mScanLines[i].posY-5);
            ofDrawBitmapString(ofToString(mScanLines[i].posY)  , 5, mScanLines[i].posY-5);
        }
    }
    
    //---------
    // Scan Edge
    //---------
    if (mScanEdges.size()) {
        ofFill();
        for (int i = 0; i < mScanEdges.size(); i++) {
            ofSetColor(mScanEdges[i].col, 255);
            int y1, y2, scanW;
            getFixedScanParam(mScanEdges[i], &y1, &y2, &scanW);
            if (scanW <= 0) continue;
            int pxSize = scanW * mCanvasWidth;
            //Draw
            unsigned char* px = new unsigned char[pxSize];
            for (int j = 0; j < pxSize; j++) {
                px[j] = mSubImg[0].getPixels()[(int)(y1 * mCanvasWidth) + j];
            }
            tEdgeImg.setFromPixels(px, mSubImg[0].width, scanW, OF_IMAGE_GRAYSCALE);
            tEdgeImg.draw(0, y1);
            delete px;
        }
    }
    
    //---------
    // Scan Pixels
    //---------
    if (mScanPixels.size() && mOrgImg.isAllocated()) {
        ofFill();
        for (int i = 0; i < mScanPixels.size(); i++) {
            ofSetColor(mScanPixels[i].col, 255);
            int resY = mResoNumY;
            int resX = mResoNumX;
            int y1, y2, scanW;
            getFixedScanParam(mScanPixels[i], &y1, &y2, &scanW);
            if (scanW <= 0) continue;
            //Draw
            if (resY <= 0) resY = 1; if (resX <= 0) resX = 1;
            for (int y = 0; y < mOrgImg.height; y += resY) {
                for (int x = 0; x < mOrgImg.width; x += resX) {
                    if (y >= y1 && y < y2) {
                        int diff = y - mScanPixels[i].posY;
                        int a = ofMap(diff, 0, mScanPixels[i].w, 255, 0);
                        ofSetColor(mOrgImg.getColor(x, y), a);
                        int w = resX;
                        int h = resY;
                        if (x + resX > mOrgImg.width) w -= (x + resX) - mOrgImg.width;
                        if (y + resY > mOrgImg.height) h -= (y + resY) - mOrgImg.height;
                        ofRect(x, y, w, h);
                    }
                }
            }
        }
    }
    
    //----------
    // Scan Histogram
    //----------
    if (mScanHistos.size() && mCurrentSubImg->isAllocated()) {
        ofFill();
        for (int i = 0; i < mScanHistos.size(); i++) {
            if (mScanHistos[i].posX <= mCurrentSubImg->width) {
                
                //first: grow line
                ofSetColor(mScanHistos[i].col);
                ofSetLineWidth(mScanHistos[i].w);
                ofLine(0, mScanHistos[i].posY, mScanHistos[i].posX, mScanHistos[i].posY);
            
            } else {
                if (mScanHistos[i].vec.empty()) {
                    
                    //second process: get histogram
                    ofSetColor(mScanHistos[i].col, 255);
                    int y1, y2, scanW;
                    getFixedScanParam(mScanHistos[i], &y1, &y2, &scanW);
                    if (scanW <= 0) continue;
                    
                    list<unsigned char> srcList;
                    const int srcCh = mCurrentSubImg->getPixelsRef().getNumChannels();
                    int j = 0;
                    for (int y = y1; y < y2; y++) {
                        for (int x = 0; x < mCurrentSubImg->width; x++) {
                            srcList.push_back(mCurrentSubImg->getColor(x, y).getHue());
                        }
                    }
                    mScanHistos[i].vec = og.getHistogram(srcList, true);
                
                } else {
                
                    //last: draw histogram as time
                    int a = ofMap(mScanHistos[i].life, 0, 2000, 0, 255, true);
                    ofSetColor(0, 255, 255, a);
                    for (int j = 0; j < mScanHistos[i].vec.size(); j++) {
                        int W = ofMap(j, 0, mScanHistos[i].vec.size(), 0, mCurrentSubImg->width);
                        float yy = mScanHistos[i].posY + ((mScanHistos[i].vec[j]) * 50);
                        ofRect(W, mScanHistos[i].posY, W+(mScanHistos[i].vec.size()/mCurrentSubImg->width), yy);
                    }
                }
            }
        }
    }
    
    //----------
    // Scan Spectrum
    //----------
    if (mScanSpectrum.size() && mOrgImg.isAllocated()) {
        ofFill();
        for (int i = 0; i < mScanSpectrum.size(); i++) {
            if (mScanSpectrum[i].posX <= mOrgImg.width) {
                
                //1) grow line
                ofSetColor(mScanSpectrum[i].col);
                ofSetLineWidth(mScanSpectrum[i].w);
                ofLine(0, mScanSpectrum[i].posY, mScanSpectrum[i].posX, mScanSpectrum[i].posY);
                
            } else {
                if (mScanSpectrum[i].vec.empty()) {
                    
                    //2-1) calc
                    ofSetColor(mScanSpectrum[i].col, 255);
                    int y1, y2, scanW;
                    getFixedScanParam(mScanSpectrum[i], &y1, &y2, &scanW);
                    if (scanW <= 0) continue;
                    
                    const int srcCh = mOrgImg.getPixelsRef().getNumChannels();
                    int j = 0;
                    for (int y = y1; y < y2; y++) {
                        for (int x = 0; x < mOrgImg.width; x++) {
                            mScanSpectrum[i].vec.push_back(mOrgImg.getColor(x, y).getBrightness() / 255);
                        }
                    }
                    
                } else {
                    
                    //2-2) draw and second scan
                    int spectW = 150;
                    if (mScanSpectrum[i].secondScanPos < mOrgImg.width) {
                        ofSetColor(255, 255);
                        ofLine(mScanSpectrum[i].secondScanPos, mScanSpectrum[i].posY - (spectW/2),
                               mScanSpectrum[i].secondScanPos, mScanSpectrum[i].posY + (spectW/2));
                        
                        //TODO: test send
                        ofxOscMessage m;
                        m.setAddress(SCAN_SPECTRUM);
                        m.addIntArg(i);
                        m.addFloatArg(mScanSpectrum[i].vec[mScanSpectrum[i].secondScanPos]);
                        sender.sendMessage(m);
                    }
                    
                    int a = ofMap(mScanSpectrum[i].life, 0, 500, 0, 255, true);
//                    ofSetColor(255, 255, 0, a);
                    for (int j = 0; j < mScanSpectrum[i].vec.size(); j++) {
                        ofSetColor(mOrgImg.getColor(j, mScanSpectrum[i].posY), a);
                        ofLine(j, mScanSpectrum[i].posY, j, mScanSpectrum[i].posY + ((mScanSpectrum[i].vec[j]-0.5)*spectW) );
                    }
                }
            }
        }
    }
    
    //-----------
    // Gaze Object
    //-----------
    if (mGazeObject.size()) {
        ofNoFill();
        for (int i = 0; i < mGazeObject.size(); i++) {
            //ofSetLineWidth((mCircleLife/255)*10);
            ofSetLineWidth(1);
            ofSetColor(mGazeObject[i].col, (int)mGazeObject[i].life);
            ofCircle(mGazeObject[i].pos, mGazeObject.size());
        }
    }
    
    //-----------
    // Future Points
    //-----------
    if (mFuturePts.size()) {
        int w = 10;
        ofSetLineWidth(1);
        for (int i = 0; i < mFuturePts.size(); i++) {
            ofSetColor(255, 0, 0, 255);
            ofLine(mFuturePts[i].x-w, mFuturePts[i].y, mFuturePts[i].x+w, mFuturePts[i].y);
            ofLine(mFuturePts[i].x, mFuturePts[i].y-w, mFuturePts[i].x, mFuturePts[i].y+w);
            ofSetColor(255, 255, 255, 255);
            ofDrawBitmapString(ofToString(mFuturePts[i].x), mFuturePts[i].x+10, mFuturePts[i].y);
        }
    }
    
    // front color mapping
    ofFill();
    ofSetColor(mFrontColor);
    ofRect(0, 0, mCanvasWidth, mCanvasHeight);
    
    ofDisableAlphaBlending();
    ofPopStyle();
}

//========== draw mapping method ==========
void Canvas::drawMapDownSampl()
{
    if (mOrgImg.isAllocated()) {
        ofFill();
        int resY = mMapDownSample.resoX;
        int resX = mMapDownSample.resoY;
        if (resY <= 0) resY = 1; if (resX <= 0) resX = 1;
        for (int y = 0; y < mOrgImg.height; y += resY) {
            for (int x = 0; x < mOrgImg.width; x += resX) {
                ofSetColor(mOrgImg.getColor(x, y));
                int w = resX;
                int h = resY;
                if (x + resX > mOrgImg.width) w -= (x + resX) - mOrgImg.width;
                if (y + resY > mOrgImg.height) h -= (y + resY) - mOrgImg.height;
                ofRect(x, y, w, h);
            }
        }
    }
}

void Canvas::drawMapHexText()
{
    if (mOrgImg.isAllocated()) {
        for (int y = 0; y < mOrgImg.height; y += 10) {
            for (int x = 0; x < mOrgImg.width; x += 64) {
                ofColor col = mOrgImg.getColor(x, y);
                ofSetColor(col);
                const int _x = (x + (int)mMapHexText.offset.x) % (int)mOrgImg.width;
                const int _y = (y + (int)mMapHexText.offset.y) % (int)mOrgImg.height;
                ofDrawBitmapString(ofToHex(col.getHex()), _x, _y);
            }
        }
    }
}

void Canvas::drawMapDetectColorBlob()
{
    if (mOrgImg.isAllocated()) {
        if (!mMapDetectColor.tMapColorBlob.bAllocated) {
            mMapDetectColor.tMapColorBlob.allocate(mOrgImg.width, mOrgImg.height);
        }
        
        float targetH = ofClamp(mMapDetectColor.pos, 0, 255);

        // もしScanningPositionが移動していた場合のみ画像を計算
        if (mMapDetectColor.pos != mMapDetectColor.tmpPos) {
            int size = mOrgImg.width * mOrgImg.height;
            unsigned char* px = new unsigned char[size];
            for (int y = 0; y < mOrgImg.height; y++) {
                for (int x = 0; x < mOrgImg.width; x++) {
                    int i = x + (y * mOrgImg.width);
                    float h = mOrgImg.getPixelsRef().getColor(x, y).getHue();
                    if (h > targetH - mMapDetectColor.threshold/2 && h < targetH + mMapDetectColor.threshold/2) {
                        px[i] = 255;
                    } else {
                        px[i] = 0;
                    }
                }
            }
            mMapDetectColor.tMapColorBlob.setFromPixels(px, mOrgImg.width, mOrgImg.height);
            mMapDetectColor.tCont.findContours(mMapDetectColor.tMapColorBlob, 50, size, 200, true);
            delete px;
            
        }
        
        
        ofSetColor(255);
        mMapDetectColor.tMapColorBlob.draw(0, 0);
        ofNoFill();
        ofSetColor(ofColor::fromHsb(targetH, 255, 255));
        if (mMapDetectColor.pos == mMapDetectColor.tmpPos) {
            for (int i = 0; i < mMapDetectColor.tCont.blobs.size(); i++) {
                ofRect(mMapDetectColor.tCont.blobs[i].boundingRect);
            }
            sendOsc("/detectColor", (float)mMapDetectColor.tCont.nBlobs);
        }

        mMapDetectColor.tmpPos = mMapDetectColor.pos;
    }
}

void Canvas::drawMapReturnScanLine()
{
    if (!mMapReturnScanLine.lines.empty()) {
        ofSetLineWidth(mMapReturnScanLine.lines[0].w);
        ofSetColor(mMapReturnScanLine.lines[0].col);
        ofLine(0, mMapReturnScanLine.lines[0].posY,
               mCanvasWidth, mMapReturnScanLine.lines[0].posY);
    }
}

void Canvas::drawMapDetectBlobs()
{
    mBlobNum = mMapDetectBlobs.blobNum;
    if (mMapDetectBlobs.pBlobs != NULL) {
        //--------------------
        //blobを描画
        //--------------------
        ofSetColor(255, 225);
        mMyBlobs[mBlobNum].blobImg.draw(0, 0);
        //            mMyBlobs[mBlobNum].cvBlob.draw();
        if (mMapDetectBlobs.isDrawLines)  {
            ofNoFill();
            ofSetHexColor(0xFF00FF);
            ofBeginShape();
            for (int i = 0; i < mMyBlobs[mBlobNum].cvBlob.nPts; i++){
                ofVertex(mMyBlobs[mBlobNum].cvBlob.pts[i].x, mMyBlobs[mBlobNum].cvBlob.pts[i].y);
            }
            ofEndShape(true);
            ofSetHexColor(0xff0000);
            ofRect(mMyBlobs[mBlobNum].cvBlob.boundingRect.x,
                   mMyBlobs[mBlobNum].cvBlob.boundingRect.y,
                   mMyBlobs[mBlobNum].cvBlob.boundingRect.width,
                   mMyBlobs[mBlobNum].cvBlob.boundingRect.height);
        
            //blobの重心の重心座標を描画
            ofSetColor(80);
            ofLine(mBlobsCenter.x-15, mBlobsCenter.y, mBlobsCenter.x+15, mBlobsCenter.y);
            ofLine(mBlobsCenter.x, mBlobsCenter.y-15, mBlobsCenter.x, mBlobsCenter.y+15);
            
            //中心からblob重心までの線を描画
            ofSetColor(255, 0, 0);
            ofPoint pos = mMyBlobs[mBlobNum].cvBlob.centroid;
            ofLine(pos.x-10, pos.y-10, pos.x+10, pos.y+10);
            ofLine(pos.x+10, pos.y-10, pos.x-10, pos.y+10);
            ofSetColor(255,255,255, 60);
            ofLine(pos.x, pos.y, mBlobsCenter.x, mBlobsCenter.y);
            
            //他のblobへの移動確率を描画
//            for (int i=0; i < mMyBlobs.size(); i++) {
//                if (i != mBlobNum) {
//                    float p = 0;
//                    for (int j=0; j < mMyBlobs[mBlobNum].moveProbs.size(); j++) {
//                        if (j == i) {
//                            p = mMyBlobs[mBlobNum].moveProbs[j];
//                        }
//                    }
//                    ofSetColor(180, 180, 230, p*255);
//                    p = ofMap(p, 0.0, 1.0, 0, 50);
//                    if (p > 0.0) {
//                        ofSetLineWidth(p);
//                        ofLine(mMyBlobs[mBlobNum].cvBlob.centroid, mMyBlobs[i].cvBlob.centroid);
//                    }
//                }
//            }
            
        }
        //ルーラーを描画
        ofSetColor(40);
        for (int i=0; i < mCanvasWidth; i += 10){
            ofLine(i, 0, i, mCanvasHeight);
        }
        for (int i=0; i < mCanvasHeight; i += 10){
            ofLine(0, i, mCanvasWidth, i);
        }
        
        if (mMapDetectBlobs.isDrawLines)  {
            ofSetColor(80);
            ofSetLineWidth(0.5);
            ofLine(mLabelImg.width/2, 0, mLabelImg.width/2, mLabelImg.height);
            ofLine(0, mLabelImg.height/2, mLabelImg.width, mLabelImg.height/2);
        
            //平均カラーを描画
            if (mOrgImg.isAllocated()) {
                ofSetColor(255);
                ofNoFill();
                ofSetLineWidth(2);
                ofCircle(ofGetWidth()-20, 20, 11);
                ofSetColor(mMyBlobs[mBlobNum].color);
                ofFill();
                ofCircle(ofGetWidth()-20, 20, 10);
            }
            
            //ルーラーを描画
            ofSetLineWidth(0.5);
            ofSetColor(200);
            for (int i=0; i < mLabelImg.width; i += 10){
                if (i%100 == 0)
                    ofLine(i, 0, i, 15);
                else
                    ofLine(i, 0, i, 10);
            }
            for (int i=0; i < mLabelImg.height; i += 10){
                if (i%100 == 0)
                    ofLine(0, i, 15, i);
                else
                    ofLine(0, i, 10, i);
            }
            
        }
        //十字のルーラーを描画
        ofSetColor(120, 150, 120, 160);
        ofLine(mMyBlobs[mBlobNum].cvBlob.centroid.x, 0, mMyBlobs[mBlobNum].cvBlob.centroid.x, mLabelImg.height);
        ofLine(0, mMyBlobs[mBlobNum].cvBlob.centroid.y, mLabelImg.width, mMyBlobs[mBlobNum].cvBlob.centroid.y);
        ofSetColor(255,255,255,180);
        ofDrawBitmapString(ofToString(mMyBlobs[mBlobNum].cvBlob.centroid.x, 2), mMyBlobs[mBlobNum].cvBlob.centroid.x+10, 25);
        ofDrawBitmapString(ofToString(mMyBlobs[mBlobNum].cvBlob.centroid.y, 2), 15, mMyBlobs[mBlobNum].cvBlob.centroid.y+10);
        
        //Dither
        ofSetLineWidth(0.5);
        ofSetColor(120, 150, 120, 127);
        if (mResoNumX > 0 && mResoNumY > 0) {
            for (int i = 0; i < mLabelImg.width; i += mResoNumX) {
                ofLine(i, 0, i, mLabelImg.height);
            }
            for (int i = 0; i < mLabelImg.height; i += mResoNumY) {
                ofLine(0, i, mLabelImg.width, i);
            }
        }
        
        //--------------------
        //情報をOSCで送る
        //--------------------
        if (mTmpBlobNum != mBlobNum) {
            ofxOscMessage m;
            m.setAddress(BLOB_OBJECT );
//            m.addIntArg(mCanvasID); //Canvas ID
//            m.addIntArg(mMyBlobs[mBlobNum].ID); //ID
//            m.addFloatArg(mMyBlobs[mBlobNum].cvBlob.centroid.x); //重心x
//            m.addFloatArg(mMyBlobs[mBlobNum].cvBlob.centroid.y); //重心y
//            m.addFloatArg(mMyBlobs[mBlobNum].cvBlob.centroid.distance(mBlobsCenter)); //中心からの距離
//            m.addFloatArg(mMyBlobs[mBlobNum].cvBlob.area); //総画素数（エリア）
//            m.addFloatArg(mMyBlobs[mBlobNum].cvBlob.length); //周辺長
            if (mOrgImg.isAllocated()) {
                m.addFloatArg((float)mMyBlobs[mBlobNum].color.r); //平均R
                m.addFloatArg((float)mMyBlobs[mBlobNum].color.g); //平均G
                m.addFloatArg((float)mMyBlobs[mBlobNum].color.b); //平均B
            } else {
                m.addFloatArg(0.0f); //平均R（空）
                m.addFloatArg(0.0f); //平均G（空）
                m.addFloatArg(0.0f); //平均B（空）
            }
            sender.sendMessage(m);
            
            //----------
            //DitherNoiseを取得
            //----------
//            mDithNoiseX.clear();
//            mDithNoiseY.clear();
//            mDithNoiseX = getDitherNoiseWavePts(mMyBlobs[mBlobNum].blobImg, mResoNumX, true);
//            mDithNoiseY = getDitherNoiseWavePts(mMyBlobs[mBlobNum].blobImg, mResoNumY, false);
            //音出力用に正規化
            //        mDithNoiseXComp.clear();
            //        vector<float>::iterator max = max_element(mDithNoiseX.begin(), mDithNoiseX.end());
            //        for (int i = 0; i < mDithNoiseX.size(); i++) {
            //            mDithNoiseXComp.push_back(mDithNoiseX[i] / (*max));
            //        }
            //        mDithNoiseCounter = 0;
            
        }
        
        // 現在のblobNumを記憶
        mTmpBlobNum = mBlobNum;
        
    }
}

//---------------------------------------------
/**
 ScanEffectで線の両端と太さが原画像の大きさからはみ出ないようにする処理.
 第1, 引数は入力値, あとは出力値.
 */
void Canvas::getFixedScanParam(const ScanLine& scanLine, int *y1, int *y2, int *scanW)
{
    (*y1) = scanLine.posY;
    (*y2) = (*y1) + scanLine.w;
    int minY = 0;
    int maxY = mCanvasHeight;
    if ((*y1) < minY) (*y1) = minY; if ((*y2) < minY) (*y2) = minY;
    if ((*y1) > maxY) (*y1) = maxY; if ((*y2) > maxY) (*y2) = maxY;
    (*scanW) = (*y2) - (*y1);
}

//---------- Make Effects ----------
void Canvas::makeScanLine(const float _speed, const float _w)
{
    if (!mScanLines.empty()) return;
    
    ScanLine tmp;
    tmp.speed = _speed;
    tmp.w = _w;
    tmp.posY = mCanvasHeight;
    tmp.col = ofColor(255,255,255);
    mScanLines.push_back(tmp);
}

void Canvas::makeScanEdge(const float _speed, const float _w)
{
    if (mSubImg.empty()) { cout << "effect image is empty" << endl; return; }
    if (!mSubImg[0].isAllocated()) { cout << "effect image is not allocated" << endl; return; }
    if (mScanEdges.size() > 2) return;
    
    ScanLine tmp;
    tmp.speed = _speed;
    tmp.w = _w;
    tmp.posY = mCanvasHeight;
    tmp.col = ofColor(255,255,255);
    mScanEdges.push_back(tmp);
}

void Canvas::makeScanPixels(const float _speed, const float _w)
{
    assert(mOrgImg.isAllocated());
    if (mScanPixels.size() > 1) return;

    ScanLine tmp;
    tmp.speed = _speed;
    tmp.w = _w;
    tmp.posY = mCanvasHeight;
    tmp.col = ofColor(255,255,255);
    mScanPixels.push_back(tmp);
}

void Canvas::makeScanHisto(const float _speed, const float _w, const float lifeTime)
{
    assert(mOrgImg.isAllocated());
    makeScanHisto(ofRandom(mCanvasHeight), _speed, _w, lifeTime);
}

void Canvas::makeScanHisto(const float _pos, const float _speed, const float _w, const float lifeTime)
{
    if (mScanHistos.size() > 10) return;
    assert(mOrgImg.isAllocated());
    if (mCurrentSubImg == NULL) return;
    
    ScanLineExtra tmp;
    tmp.speed = _speed;
    tmp.w = _w;
    tmp.posY = ofClamp(_pos, 0, mCanvasHeight);
    tmp.posX = 0;
    tmp.life = lifeTime;
    tmp.col = ofColor(255,255,255);
    tmp.secondScanPos = 0;
    tmp.secondScanSpeed = 1;
    mScanHistos.push_back(tmp);
}

void Canvas::makeScanSpectrum(const float _pos, const float _speed, const float _w, const float lifeTime)
{
    if (mScanSpectrum.size() > 10) return;
    assert(mOrgImg.isAllocated());
    if (mCurrentSubImg == NULL) return;
    
    ScanLineExtra tmp;
    tmp.speed = _speed;
    tmp.w = _w;
    tmp.posY = ofClamp(_pos, 0, mCanvasHeight);
    tmp.posX = 0;
    tmp.life = lifeTime;
    tmp.col = ofColor(255,255,255);
    tmp.secondScanPos = 0;
    tmp.secondScanSpeed = mCanvasWidth / 6;
    mScanSpectrum.push_back(tmp);
}

void Canvas::makeGazeObjects(const ofPoint &_pos, const float _size, const ofColor _col, const float _decay)
{
    if (mBlobEffects.size() > 10) return;

    BlobObject tmp;
    tmp.pos.set(_pos);
    tmp.life = 255;
    tmp.decay = _decay;
    tmp.size = _size;
    tmp.col = _col;
//    mBlobEffects.push_back(tmp);
}

void Canvas::ClearAllEffects()
{
    mScanLines.clear();
    mScanEdges.clear();
    mScanPixels.clear();
    mScanHistos.clear();
    mScanSpectrum.clear();
    mBlobEffects.clear();
}

//----------
// disable all mapping effect
void Canvas::setMap()
{
    mMapDownSample.enable = false;
    mMapHexText.enable = false;
    mMapDetectColor.enable = false;
    mMapReturnScanLine.enable = false;
    mMapReturnScanLine.lines.clear();
    mMapDetectBlobs.enable = false;
}

void Canvas::setMapDownSample(const int _resoX, const int _resoY)
{
    if (mOrgImg.isAllocated()) {
        mMapDownSample.enable = true;
        mMapDownSample.resoX = _resoX;
        mMapDownSample.resoY = _resoY;
    } else {
        cout << "[ERROR:mapDownSampl] no allocated original image" << endl;
    }
}

void Canvas::setMapHexText(const ofPoint _offset)
{
    if (mOrgImg.isAllocated()) {
        mMapHexText.enable = true;
        mMapHexText.offset = _offset;
    } else {
        cout << "[ERROR:mapHexText] no allocated original image" << endl;
    }
}

void Canvas::setMapDetectColor(const float _speed, MapDetectColor::_mode _mode)
{
    if (mOrgImg.isAllocated()) {
        mMapDetectColor.enable = true;
        mMapDetectColor.speed = _speed;
        mMapDetectColor.mode = _mode;
    } else {
        cout << "[ERROR:mapDetectColor] no allocated original image" << endl;
    }
}

void Canvas::setMapReturnScanLine(const float _speed, const float _w)
{
    mMapReturnScanLine.enable = true;
    if (mMapReturnScanLine.lines.empty()) {
        ScanLine line;
        line.speed = _speed;
        line.w = _w;
        line.posY = mCanvasHeight-3;
        line.col = ofColor(255);
        mMapReturnScanLine.lines.push_back(line);
    } else {
        mMapReturnScanLine.lines[0].speed = _speed;
        mMapReturnScanLine.lines[0].w = _w;
    }
}

void Canvas::setMapDetectBlobs(const int blobNum, const bool isDrawLines)
{
    if (!mMyBlobs.empty()) {
        mMapDetectBlobs.enable = true;
        int num = blobNum;
        if (blobNum < 0) num = 0;
        if (blobNum > mMaxBlob) num = mMaxBlob;
        mMapDetectBlobs.blobNum = num;
        mMapDetectBlobs.isDrawLines = isDrawLines;
        if (mMapDetectBlobs.pBlobs == NULL) {
            mMapDetectBlobs.pBlobs = &mMyBlobs;
        }
    }
}

//----------

void Canvas::sendAvgCol()
{
    ofxOscMessage m;
    m.setAddress(AVG_COLOR);
    m.addFloatArg(mImageParams.avgColor.r);
    m.addFloatArg(mImageParams.avgColor.g);
    m.addFloatArg(mImageParams.avgColor.b);
    sender.sendMessage(m);
}

void Canvas::sendHistoR(const int num)
{
    int _num = num % mImageParams.histR.size();
    ofxOscMessage m;
    m.setAddress(HISTO_R);
    m.addFloatArg(mImageParams.histR[_num]);
    sender.sendMessage(m);
}

void Canvas::sendHistoG(const int num)
{
    int _num = num % mImageParams.histR.size();
    ofxOscMessage m;
    m.setAddress(HISTO_G);
    m.addFloatArg(mImageParams.histG[_num]);
    sender.sendMessage(m);
}

void Canvas::sendHistoB(const int num)
{
    int _num = num % mImageParams.histR.size();
    ofxOscMessage m;
    m.setAddress(HISTO_B);
    m.addFloatArg(mImageParams.histB[_num]);
    sender.sendMessage(m);
}


//----------

/** 特徴点抽出
 reference: http://opencv.jp/cookbook/opencv_img.html#id44
 */
void Canvas::detectFuturePoints(ofImage& src_img)
{
    //check
    if (!src_img.isAllocated()) {
        cout << "[ERROR] faild detect future points" << endl;
        return;
    }
    
    mFuturePts.clear();
    
    //    cv::Mat img = cv::imread(mSrcImgPath, 1);
    cv::Mat img;
    img = toCv(src_img.getPixelsRef());
    if(img.empty()) return -1;
    
    cv::Mat gray_img;
    cv::cvtColor(img, gray_img, CV_BGR2GRAY);
    cv::normalize(gray_img, gray_img, 0, 255, cv::NORM_MINMAX);
    
    std::vector<cv::KeyPoint> keypoints;
    std::vector<cv::KeyPoint>::iterator itk;
    
    // 固有値に基づく特徴点検出
    // maxCorners=80, qualityLevel=0.01, minDistance=5, blockSize=3
    cv::GoodFeaturesToTrackDetector detector(200, (double)ofRandom(0.01, 0.1), 5, 3);
    detector.detect(gray_img, keypoints);
//    cv::Scalar color(0,200,255);
    if (keypoints.size() > 100) {
        cout << "future point over num" << endl;
        return;
    }
    for(itk = keypoints.begin(); itk!=keypoints.end(); ++itk) {
        ofVec2f tmp = toOf(itk->pt);
        mFuturePts.push_back(ofPoint(tmp.x, tmp.y));
    }
}

//====================
// OSC sender
//====================
void Canvas::sendOsc(const string& addr, const float arg1)
{
    ofxOscMessage m;
    m.setAddress(addr);
    m.addFloatArg(arg1);
    sender.sendMessage(m);
}


void Canvas::sendOsc(const string& addr, const float arg1, const float arg2)
{
    ofxOscMessage m;
    m.setAddress(addr);
    m.addFloatArg(arg1);
    m.addFloatArg(arg2);
    sender.sendMessage(m);
}

void Canvas::sendOsc(const string& addr, const float arg1, const float arg2, const float arg3)
{
    ofxOscMessage m;
    m.setAddress(addr);
    m.addFloatArg(arg1);
    m.addFloatArg(arg2);
    m.addFloatArg(arg3);
    sender.sendMessage(m);
}

void Canvas::sendOsc(const string &addr, const string &argStr)
{
    ofxOscMessage m;
    m.setAddress(addr);
    m.addStringArg(argStr);
    sender.sendMessage(m);
}
