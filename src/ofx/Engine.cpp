
#include "Engine.h"

#ifdef __ANDROID__
#include "ofxAndroid.h"
#endif

pdsp::Engine::Engine() : score( sequencer ){
    inputID = 0;
    outputID = 0;
    inStreamActive = false;
    outStreamActive = false;

    inputChannels = outputChannels = -1; // for the next method to work
    setChannels(0, 2);

    state = closedState;
    
#if defined(TARGET_WIN32)
    api = ofSoundDevice::Api::MS_DS;
#elif defined(TARGET_OSX)
    api = ofSoundDevice::Api::OSX_CORE;
#else
    api = ofSoundDevice::Api::PULSE;
#endif


#ifndef __ANDROID__
    midiIns.reserve(10);
    controllers.reserve(20);
    controllerLinkedMidis.reserve(20);

    midiIns.clear();
    controllers.clear();
    hasMidiIn = false;
#endif

    bBackgroundAudio = false;

}


pdsp::Engine::~Engine(){
    if(state!= closedState){
        close();
    }
}

void pdsp::Engine::onExit( ofEventArgs &args ){
    if(state!= closedState){
        close();
        for( int in=0; in<inputChannels; in++ ){
            audio_in(in).disconnectAll();
        }
        for( int out=0; out<outputChannels; out++ ){
            audio_out(out).disconnectAll();
        }
    }
}

std::vector<ofSoundDevice> pdsp::Engine::listDevices(){
    return outputStream.getDeviceList(api);
}

void pdsp::Engine::setDeviceID(int deviceID){
    outputID = deviceID;
    inputID = deviceID;
}

void pdsp::Engine::setOutputDeviceID(int deviceID){
    outputID = deviceID;
}

void pdsp::Engine::setInputDeviceID(int deviceID){
    inputID = deviceID;
}

pdsp::Patchable& pdsp::Engine::audio_out( int channel ){
    
    if(channel < 0 ) channel = 0;
    int maxChannels = processor.channels.size();
  
    if(channel >= maxChannels){
        std::cout<<"[pdsp] warning! requested audio output channel out of range, clamping to max\n";
        #ifdef NDEBUG
        std::cout<<"[pdsp] build in debug mode for triggering an assert\n";
        #endif 
        assert(false);
        channel = maxChannels-1;
    }
    
    return processor.channels[channel];
    
}

pdsp::Patchable& pdsp::Engine::audio_in( int channel ){
    
    if(channel < 0 ) channel = 0;
    int maxChannels = inputs.size();
  
    if(channel >= maxChannels){
        std::cout<<"[pdsp] warning! requested audio input channel out of range, clamping to max\n";
        #ifdef NDEBUG
        std::cout<<"[pdsp] build in debug mode for triggering an assert\n";
        #endif 
        assert(false);
        channel = maxChannels-1;
    }
    
    return inputs[channel].out_signal();
    
}

pdsp::Patchable& pdsp::Engine::blackhole( ) {
    return processor.blackhole;
}

void pdsp::Engine::setChannels( int inputChannels, int outputChannels ){
    if(this->inputChannels != inputChannels){
        this->inputChannels = inputChannels;
        inputs.resize(inputChannels);   
    }
    
    if(this->outputChannels != outputChannels){
        this->outputChannels = outputChannels;
    }

}

void pdsp::Engine::setup( int sampleRate, int bufferSize, int nBuffers){
 
    //ofAddListener( ofEvents().exit, this, &pdsp::Engine::onExit );
    
    ofLogNotice()<<"[pdsp] engine: starting with parameters: buffer size = "<<bufferSize<<" | sample rate = "<<sampleRate<<" | "<<inputChannels<<" inputs | "<<outputChannels<<" outputs\n";
   
    if ( nBuffers < 1 ) nBuffers = 1;
    
    // close if engine is already running with another settings
    if(state!=closedState){ 
        ofLogNotice()<<"[pdsp] engine: changing setup, shutting down stream";
        stop();
        if( inStreamActive ){
            inputStream.close();
        }
        if( outStreamActive ){
            outputStream.close();
        }
        
        ofSoundStreamClose(); 
        
        ofLogNotice()<<"[pdsp] engine: changing setup, releasing resources...";
        pdsp::releaseAll();
        
        state = closedState;
        ofSleepMillis( 20 );
        ofLogNotice()<<"...done";
    }
    
    // prepare all the units / modules
    pdsp::prepareAllToPlay(bufferSize, static_cast<double>(sampleRate) );


    // starts engine
    
#if (OF_VERSION_MINOR <= 9) // OF LEGACY for 0.9.8
    if(outputChannels > 0 && inputChannels == 0){
        outputStream.setOutput(*this);   
        outStreamActive = true; 
        outputStream.setDeviceID( outputID );
        outputStream.setup(outputChannels, 0, sampleRate, bufferSize, nBuffers);
    
    }else if(inputChannels > 0 && outputChannels == 0){
        inputStream.setInput(*this);  
        inStreamActive = true;
        inputStream.setDeviceID( inputID );
        inputStream.setup(0, inputChannels, sampleRate, bufferSize, nBuffers);
        
    }else{
        if( inputID == outputID ) {
            outputStream.setInput(*this);  
            outputStream.setOutput(*this);  
            outStreamActive = true;
            outputStream.setDeviceID( outputID );
            outputStream.setup(outputChannels, inputChannels, sampleRate, bufferSize, nBuffers);
        } else {
            inputStream.setInput(*this);  
            outputStream.setOutput(*this);  
            outStreamActive = true;
            inStreamActive  = true;
            outputStream.setDeviceID( outputID );
            outputStream.setup(outputChannels, 0, sampleRate, bufferSize, nBuffers);
            inputStream.setDeviceID( inputID );
            inputStream.setup(0, inputChannels, sampleRate, bufferSize, nBuffers);
        }     
    }
#else // END OF LEGACY 0.9.8

    // OF MASTER VERSION
    
	ofSoundStreamSettings settings;
	settings.sampleRate = (size_t)  sampleRate;
	settings.bufferSize = (size_t)  bufferSize;
    settings.numBuffers = (size_t)  nBuffers;
	settings.numOutputChannels = (size_t) outputChannels;
	settings.numInputChannels = (size_t)  inputChannels;
    
    settings.setApi( api );
    
    #if defined(__ANDROID__)
        if( outputChannels > 0 ){
            outStreamActive = true;
            settings.setOutListener(this);
        }
        if(inputChannels > 0 ){
            outStreamActive = true;
            settings.setInListener(this);
            ofxAndroidRequestPermission(OFX_ANDROID_PERMISSION_RECORD_AUDIO);
        }
        outputStream.setup( settings ); 

    #elif defined(TARGET_OF_IOS)
        if( outputChannels > 0 ){
            settings.setOutListener(this);
        }
        if(inputChannels > 0 ){
            settings.setInListener(this);
        }

        if( bBackgroundAudio ){ ofxiOSSoundStream::setMixWithOtherApps(true); }
        ofSoundStreamSetup( settings );

    #else


        auto devices = outputStream.getDeviceList(api);
        
        if( outputChannels > 0 ){
            outStreamActive = true;
            settings.setOutListener(this);
            settings.setOutDevice( devices[outputID] );
        }
        if(inputChannels > 0 ){
            outStreamActive = true;
            settings.setInListener(this);
            settings.setInDevice( devices[inputID] );
        }
        
        outputStream.setup( settings );

    #endif

#endif // END OF MASTER VERSION

    if( outputChannels > 0 ){
        84.0f >> testOscillator.in_pitch();
        testOscillator >> testAmp >> processor.channels[0];
    }

    state = startedState;

    #ifndef TARGET_OF_IOS
    ofLogNotice()<<"[pdsp] engine: started | buffer size = "<<outputStream.getBufferSize()<<" | sample rate = "<<outputStream.getSampleRate()<<" | "<<outputStream.getNumInputChannels()<<" inputs | "<<outputStream.getNumOutputChannels()<<" outputs\n";
    #endif
    
}

void pdsp::Engine::start(){
    if(inStreamActive && state < startedState){
        inputStream.start();
    }
    if(outStreamActive && state < startedState){
        outputStream.start();
    }
    #ifdef TARGET_OF_IOS
    ofSoundStreamStart();
    #endif    
    state = startedState;
}

void pdsp::Engine::stop(){
    if(state==startedState){
        if( inStreamActive ){
            inputStream.stop();
        }
        if( outStreamActive ){
            outputStream.stop();
        }    

        ofSoundStreamStop();	

        state = stoppedState;
    }
}

void pdsp::Engine::close(){
    
    std::cout<<"[pdsp] engine: closing...\n";
    
    stop();

#ifndef __ANDROID__
    if(hasMidiIn){
        for( pdsp::midi::Input* & in : midiIns ){
            in->closePort();
        } 
    }
#endif

    for (pdsp::osc::Input* & in : pdsp::osc::Input::instances) {
        in->close();
    }

    for(pdsp::ExtSequencer* & out : ExtSequencer::instances) {
        out->close();
    }

    if( inStreamActive ){
        inputStream.close();
    }
    if( outStreamActive ){
        outputStream.close();
    }
    
    ofSoundStreamClose();

    pdsp::releaseAll();
    
    state = closedState;
    std::cout<<"[pdsp] engine: audio streams closed and resources released\n";    

}

void pdsp::Engine::audioOut(ofSoundBuffer &outBuffer) {
    
    int bufferSize =  outBuffer.getNumFrames() ;

#ifndef __ANDROID__
    // midi input processing
    if(hasMidiIn){
        for( pdsp::midi::Input* & in : midiIns){
            in->processMidi( bufferSize );
        } 
        for(int i=0; i<(int)controllers.size(); ++i){
            controllers[i]->processMidi( *(controllerLinkedMidis[i]), bufferSize );
        }
    }
#endif

    for (pdsp::osc::Input* & in : pdsp::osc::Input::instances) {
        in->processOsc(bufferSize);
        if (in->hasTempoChange()) {
            sequencer.setTempo(in->getTempo());
        }
    }
   
    // score and playhead processing
    sequencer.process( bufferSize );
 
    // external outputs processing
    for(pdsp::ExtSequencer* & out : ExtSequencer::instances) {
        out->process(bufferSize);
    }
    
    //DSP processing
    if(outputChannels > 0){
        processor.processAndCopyInterleaved(outBuffer.getBuffer().data(), outBuffer.getNumChannels(), outBuffer.getNumFrames());    
    }
}

void pdsp::Engine::audioIn (ofSoundBuffer &inBuffer) {
    for( int i=0; i<inputChannels; i++){
        inputs[i].copyInterleavedInput( inBuffer.getBuffer().data(), i,  inBuffer.getNumChannels(), inBuffer.getNumFrames());
    }
}

#ifndef __ANDROID__
void pdsp::Engine::addMidiController( pdsp::Controller & controller, pdsp::midi::Input & midiIn ){
    
    bool midiInFound = false;
    for( pdsp::midi::Input* & ptr : midiIns ){
        if( ptr == &midiIn ){
            midiInFound = true;
        } 
    }
    if( ! midiInFound ){
        midiIns.push_back( &midiIn );
    }
    
    bool midiControllerFound = false;
    
    for( pdsp::Controller* & ptr : controllers ){
        if( ptr == &controller ){
            midiControllerFound = true;
            std::cout<<"[pdsp] warning! you have already added this controller, you shouldn't add it twice\n";
            pdsp::pdsp_trace();
        } 
    }
    if( ! midiControllerFound ){
        controllers.push_back( &controller );
        controllerLinkedMidis.push_back( &midiIn );
    }    
    hasMidiIn = true;
    
}

void pdsp::Engine::addMidiOut( pdsp::midi::Output & midiOut ) {}
#endif

#ifndef TARGET_OF_IOS
#ifndef __ANDROID__
void pdsp::Engine::addSerialOut( pdsp::serial::Output & serialOut ) {}
#endif
#endif

void pdsp::Engine::addExternalOut( pdsp::ExtSequencer & externalOut ) {}

void pdsp::Engine::addOscInput( pdsp::osc::Input & oscInput ) {}

void pdsp::Engine::test( bool testingActive, float testingDB ){
    if( testingActive ){
        DBtoLin::eval(testingDB) >> testAmp.in_mod();
    }else{
        0.0f >> testAmp.in_mod();
    }
}

pdsp::Patchable & pdsp::Engine::out_bar_ms(){
    return barTime.out_signal();
}

void pdsp::Engine::setBackgroundAudio( bool active ){
    bBackgroundAudio = active;
}

void pdsp::Engine::setApi( ofSoundDevice::Api api ){
    this->api = api;
}
