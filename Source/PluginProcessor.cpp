#include "PluginProcessor.h"
#include "PluginEditor.h"
SessionPhotosProcessor::SessionPhotosProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),server(store,[this]{modified();}){}
SessionPhotosProcessor::~SessionPhotosProcessor(){server.stop();cancelPendingUpdate();}
void SessionPhotosProcessor::setStateInformation(const void* data,int size){restoreFailed= size<0 || !store.restore(data,size_t(size));}
void SessionPhotosProcessor::handleAsyncUpdate(){updateHostDisplay(ChangeDetails().withNonParameterStateChanged(true));}
juce::AudioProcessorEditor* SessionPhotosProcessor::createEditor(){return new SessionPhotosEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SessionPhotosProcessor();}
