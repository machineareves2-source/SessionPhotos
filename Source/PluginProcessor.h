#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LocalServer.h"
class SessionPhotosProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater {
public:
    SessionPhotosProcessor();
    ~SessionPhotosProcessor() override;
    const juce::String getName() const override {return "Session Photos";}
    void prepareToPlay(double,int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override {return l.getMainInputChannelSet()==l.getMainOutputChannelSet() && !l.getMainOutputChannelSet().isDisabled();}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override {}
    bool hasEditor() const override {return true;}
    juce::AudioProcessorEditor* createEditor() override;
    double getTailLengthSeconds() const override {return 0;}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {return {};}
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& dest) override {store.save(dest);}
    void setStateInformation(const void* data,int size) override;
    void modified(){triggerAsyncUpdate();}
    PhotoStore store;
    LocalServer server;
    std::atomic<bool> restoreFailed{false};
private:
    void handleAsyncUpdate() override;
};
