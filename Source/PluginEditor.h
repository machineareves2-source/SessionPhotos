#pragma once
#include "PluginProcessor.h"
#include "qrcodegen.hpp"
class PhotoCanvas final : public juce::Component {
public:
    void setPhoto(std::shared_ptr<const Photo>);
    void paint(juce::Graphics&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    void mouseDown(const juce::MouseEvent& e) override {dragStart=pan;mouseStart=e.position;}
    void mouseDrag(const juce::MouseEvent& e) override {pan=dragStart+e.position-mouseStart;repaint();}
    void fit(){zoom=1;pan={};repaint();}
    void zoomBy(float f){zoom=juce::jlimit(1.f,8.f,zoom*f);repaint();}
private:
    std::shared_ptr<const Photo> photo;
    juce::Image image;
    float zoom=1;
    juce::Point<float> pan,dragStart,mouseStart;
};
class SessionPhotosEditor final : public juce::AudioProcessorEditor,private juce::Timer,private juce::ListBoxModel {
public:
    explicit SessionPhotosEditor(SessionPhotosProcessor&);
    ~SessionPhotosEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    SessionPhotosProcessor& processor;
    juce::LookAndFeel_V4 look;
    PhotoStore::Snapshot state;
    juce::ListBox list;
    PhotoCanvas canvas;
    juce::TextEditor title,notes,link;
    juce::ComboBox network;
    juce::TextButton add{"Importer"},remove{"Supprimer"},rotate{"Tourner"},fit{"Ajuster"},plus{"+"},minus{"-"},connect{"Activer le transfert"},copy{"Copier le lien"};
    juce::Label status,networkHelp;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> qrArea,phoneArea;
    std::unique_ptr<qrcodegen::QrCode> qr;
    juce::String selectedId;
    bool populating=false;
    int getNumRows() override {return int(state.photos.size());}
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    void selectedRowsChanged(int) override;
    void timerCallback() override;
    void refresh();
    void showSelected();
    void editSelected();
    void toggleServer();
    void importFiles();
    std::shared_ptr<const Photo> selected() const;
};
