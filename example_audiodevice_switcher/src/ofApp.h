#pragma once

#include "ofMain.h"
#include "ofxPDSP.h"
#include "ofxImGui.h"
#include "imgui_plot.h"

class ofApp : public ofBaseApp, pdsp::Wrapper{

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);


    void reloadEngine(int inDev, int outDev);

    // pdsp modules
    pdsp::Engine                        *engine;
    ofSoundBuffer                       inputBuffer;
    ofSoundBuffer                       emptyBuffer;

    vector<ofSoundDevice>               audioDevices;
    vector<string>                      audioDevicesStringIN;
    vector<string>                      audioDevicesStringOUT;
    vector<int>                         audioDevicesID_IN;
    vector<int>                         audioDevicesID_OUT;
    ofSoundStream                       soundStreamIN;
    ofSoundBuffer                       lastInputBuffer;

    int                                 audioINDev;
    int                                 audioOUTDev;
    int                                 audioGUIINIndex;
    int                                 audioGUIOUTIndex;
    int                                 audioSampleRate;
    int                                 audioBufferSize;

    // GUI
    ofxImGui::Gui                       gui;
    float                               input_scope[1024];

private:
    void audioProcess(float *input, int bufferSize, int nChannels);

    mutable ofMutex         audio_mutex;

};

//--------------------------------------------------------------
inline void drawWaveform(ImDrawList* drawList, ImVec2 dim, float* data, int dataSize, float thickness, ImU32 color){
    // draw signal background
    drawList->AddRectFilled(ImGui::GetWindowPos(),ImGui::GetWindowPos()+dim,IM_COL32_BLACK);

    // draw signal plot
    ImGuiEx::PlotConfig conf;
    conf.values.ys = data;
    conf.values.count = dataSize;
    conf.values.color = color;
    conf.scale.min = -1;
    conf.scale.max = 1;
    conf.tooltip.show = false;
    conf.tooltip.format = "x=%.2f, y=%.2f";
    conf.grid_x.show = false;
    conf.grid_y.show = false;
    conf.frame_size = ImVec2(dim.x, dim.y);
    conf.line_thickness = thickness;

    ImGuiEx::Plot("plot", conf);
}

//--------------------------------------------------------------
static inline float hardClip(float x){
    float x1 = fabsf(x + 1.0f);
    float x2 = fabsf(x - 1.0f);

    return 0.5f * (x1 - x2);
}
