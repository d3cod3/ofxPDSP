#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    audioSampleRate = 44100;
    audioBufferSize = 1024;

    //--------PATCHING-------
    reloadEngine(2,1);

    // ------- GUI -------
    gui.setup();

    for(int i=0;i<audioBufferSize;i++){
        input_scope[i] = 0.0f;
    }

}

//--------------------------------------------------------------
void ofApp::update(){

    std::lock_guard<std::mutex> lck(audio_mutex);

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);

    gui.begin();

    // draw waveform
    ImGui::SetNextWindowSize(ImVec2(320,180), ImGuiCond_Appearing);

    if(ImGui::Begin("Audio Device") ){
        drawWaveform(ImGui::GetWindowDrawList(), ImVec2(320,120), input_scope, 1024, 1.3f, IM_COL32(255,255,120,255));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Spacing();
        static int inDev = audioGUIINIndex;
        if(ofxImGui::VectorCombo("Input Device", &inDev,audioDevicesStringIN)){
            reloadEngine(inDev,audioOUTDev);
        }
        static int outDev = audioGUIOUTIndex;
        if(ofxImGui::VectorCombo("Output Device", &outDev,audioDevicesStringOUT)){
            reloadEngine(audioINDev,outDev);
        }

        ImGui::End();

    }

    gui.end();


}

//--------------------------------------------------------------
void ofApp::reloadEngine(int inDev, int outDev){

    audioINDev = inDev;
    audioOUTDev = outDev;

    delete engine;
    engine = nullptr;
    engine = new pdsp::Engine();

    audioDevices = soundStreamIN.getDeviceList();
    audioDevicesStringIN.clear();
    audioDevicesID_IN.clear();
    audioDevicesStringOUT.clear();
    audioDevicesID_OUT.clear();
    ofLog(OF_LOG_NOTICE,"------------------- AUDIO DEVICES");
    for(size_t i=0;i<audioDevices.size();i++){
        if(audioDevices[i].inputChannels > 0){
            audioDevicesStringIN.push_back("  "+audioDevices[i].name);
            audioDevicesID_IN.push_back(i);
            //ofLog(OF_LOG_NOTICE,"INPUT Device[%zu]: %s (IN:%i - OUT:%i)",i,audioDevices[i].name.c_str(),audioDevices[i].inputChannels,audioDevices[i].outputChannels);

        }
        if(audioDevices[i].outputChannels > 0){
            audioDevicesStringOUT.push_back("  "+audioDevices[i].name);
            audioDevicesID_OUT.push_back(i);
            //ofLog(OF_LOG_NOTICE,"OUTPUT Device[%zu]: %s (IN:%i - OUT:%i)",i,audioDevices[i].name.c_str(),audioDevices[i].inputChannels,audioDevices[i].outputChannels);
        }
        string tempSR = "";
        for(size_t sr=0;sr<audioDevices[i].sampleRates.size();sr++){
            if(sr < audioDevices[i].sampleRates.size()-1){
                tempSR += ofToString(audioDevices[i].sampleRates.at(sr))+", ";
            }else{
                tempSR += ofToString(audioDevices[i].sampleRates.at(sr));
            }
        }
        ofLog(OF_LOG_NOTICE,"Device[%zu]: %s (IN:%i - OUT:%i), Sample Rates: %s",i,audioDevices[i].name.c_str(),audioDevices[i].inputChannels,audioDevices[i].outputChannels,tempSR.c_str());
    }

    // check audio devices index
    audioGUIINIndex         = -1;
    audioGUIOUTIndex        = -1;

    for(size_t i=0;i<audioDevicesID_IN.size();i++){
        if(audioDevicesID_IN.at(i) == audioINDev){
            audioGUIINIndex = i;
            break;
        }
    }
    if(audioGUIINIndex == -1){
        audioGUIINIndex = 0;
        audioINDev = audioDevicesID_IN.at(audioGUIINIndex);
    }
    for(size_t i=0;i<audioDevicesID_OUT.size();i++){
        if(audioDevicesID_OUT.at(i) == audioOUTDev){
            audioGUIOUTIndex = i;
            break;
        }
    }
    if(audioGUIOUTIndex == -1){
        audioGUIOUTIndex = 0;
        audioOUTDev = audioDevicesID_OUT.at(audioGUIOUTIndex);
    }

    audioSampleRate = audioDevices[audioOUTDev].sampleRates[0];

    if(audioSampleRate < 44100){
        audioSampleRate = 44100;
    }

    ofLog(OF_LOG_NOTICE,"\nStarting DSP Engine on INPUT device: %s and OUTPUT device: %s\n",audioDevices[audioINDev].name.c_str(),audioDevices[audioOUTDev].name.c_str());

    engine->setChannels(audioDevices[audioINDev].inputChannels, audioDevices[audioOUTDev].outputChannels);
    this->setChannels(audioDevices[audioINDev].inputChannels,0);

    for(unsigned int in=0;in<audioDevices[audioINDev].inputChannels;in++){
        engine->audio_in(in) >> this->in(in);
    }
    this->out_silent() >> engine->blackhole();

    engine->setOutputDeviceID(audioDevices[audioOUTDev].deviceID);
    engine->setInputDeviceID(audioDevices[audioINDev].deviceID);
    engine->setup(audioSampleRate, audioBufferSize, 3);

}

//--------------------------------------------------------------
void ofApp::audioProcess(float *input, int bufferSize, int nChannels){
    if(audioSampleRate != 0){
        std::lock_guard<std::mutex> lck(audio_mutex);

        if(audioDevices[audioINDev].inputChannels > 0){
            inputBuffer.copyFrom(input, bufferSize, nChannels, audioSampleRate);
            // compute audio input
            for(size_t i = 0; i < inputBuffer.getNumFrames(); i++) {
                float sample = inputBuffer.getSample(i,0);
                input_scope[i] = hardClip(sample);
            }

            lastInputBuffer = inputBuffer;
        }
        if(audioDevices[audioOUTDev].outputChannels > 0){
            // compute audio output

        }
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
