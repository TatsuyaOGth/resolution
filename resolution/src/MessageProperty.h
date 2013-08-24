#pragma once

namespace myMess
{
    //----------
    // Send OSC Message(Address)
    //----------
    static const string SCAN_SPECTRUM = "/scanSpectrum";
    static const string BLOB_OBJECT = "/blob";
    
    static const string SCAN_LINE_GET_HALF = "/get_half";
    static const string AVG_COLOR = "/avg_color";
    static const string HISTO_R = "/histo_r";
    static const string HISTO_G = "/histo_g";
    static const string HISTO_B = "/histo_b";
    
    //----------
    // Recieve OSC Message(Address)
    //----------
    // target canvas
    static const string CANVAS_01 = "canvas0";
    static const string CANVAS_02 = "canvas1";
    
    static const string BACK_COLOR_MAP = "backColor";
    static const string FRONT_COLOR_MAP = "frontColor";
    
    // draw image
    static const string DRAW_ORG_IMAGE      = "orgimg";
    static const string DRAW_LABEL_IMAGE    = "labelimg";
    static const string DRAW_SUB_IMAGE_01   = "subimg_1";
    static const string DRAW_SUB_IMAGE_02   = "subimg_2";
    static const string DRAW_SUB_IMAGE_03   = "subimg_3";
    static const string DRAW_SUB_IMAGE_04   = "subimg_4";
    static const string DRAW_SUB_IMAGE_05   = "subimg_5";
    static const string DRAW_SUB_IMAGE_06   = "subimg_6";
    static const string DRAW_SUB_IMAGE_07   = "subimg_7";
    static const string DRAW_SUB_IMAGE_08   = "subimg_8";
    static const string DRAW_SUB_IMAGE_09   = "subimg_9";
    static const string DRAW_SUB_IMAGE_10   = "subimg_10";
    
    // scan effects
    static const string VFX_SCAN_LINE       = "scanLine";
    static const string VFX_SCAN_EDGE       = "scanEdge";
    static const string VFX_SCAN_PIX        = "scanPixel";
    static const string VFX_SCAN_HISTO      = "scanHisto";
    static const string VFX_SCAN_SPECTRUM   = "scanSpect";
    static const string VFX_SCAN_SPECT_AT_RETLINE = "scanSpectAtRetLine";
    static const string VFX_SCAN_ALL_CLEAR  = "scanClear";
    
    // Mapping Effect
    static const string VFX_MAP_SET                 = "map_set";
    static const string VFX_MAP_DOWN_SAMPLE         = "map_downsample";
    static const string VFX_MAP_HEX_TEXT            = "map_hextext";
    static const string VFX_MAP_DETECT_COLOR        = "map_detectcolor";
    static const string VFX_MAP_RETURN_SCAN_LINE    = "map_returnscanline";
    static const string VFX_MAP_DETECT_BLOBS        = "map_blobs";
    
    // Image Patrameter
    static const string REQ_AVG_COLOR = "req_avg_color";
    static const string REQ_HISTO_R = "req_histo_r";
    static const string REQ_HISTO_G = "req_histo_g";
    static const string REQ_HISTO_B = "req_histo_b";
    
}
