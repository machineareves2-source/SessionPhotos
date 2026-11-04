#include "PluginEditor.h"
using namespace juce;
namespace { const Colour bg(Colours::black),panel(0xff181818),accent(0xffa6e1cc),muted(0xff9eafb8),ink(0xfff1f5f3); }
void PhotoCanvas::setPhoto(std::shared_ptr<const Photo> p){
    const bool same=photo && p && photo->id==p->id && photo->rotation==p->rotation;
    photo=p;if(same)return;
    fit();image={};
    if(p){auto src=ImageFileFormat::loadFrom(p->bytes.getData(),p->bytes.getSize());if(p->rotation==0)image=src;else{
        const int w=src.getWidth(),h=src.getHeight();image=Image(Image::RGB,p->rotation%2?h:w,p->rotation%2?w:h,true);Graphics g(image);
        auto t=AffineTransform::rotation(float(p->rotation)*MathConstants<float>::halfPi);
        if(p->rotation==1)t=t.translated(float(h),0);if(p->rotation==2)t=t.translated(float(w),float(h));if(p->rotation==3)t=t.translated(0,float(w));
        g.drawImageTransformed(src,t);
    }}repaint();
}
void PhotoCanvas::paint(Graphics& g){g.fillAll(bg);if(!image.isValid()){g.setColour(muted);g.setFont(18);g.drawFittedText(String::fromUTF8("Vos photos de session\n\nImportez une image ou activez\nle transfert depuis votre téléphone."),getLocalBounds().reduced(30),Justification::centred,5);return;}
    g.setImageResamplingQuality(Graphics::highResamplingQuality);float scale=jmin(float(getWidth()-24)/image.getWidth(),float(getHeight()-24)/image.getHeight())*zoom;
    g.drawImageTransformed(image,AffineTransform::scale(scale).translated((getWidth()-image.getWidth()*scale)/2+pan.x,(getHeight()-image.getHeight()*scale)/2+pan.y));
}
void PhotoCanvas::mouseWheelMove(const MouseEvent&,const MouseWheelDetails& wheel){zoomBy(wheel.deltaY>0?1.15f:1/1.15f);}
SessionPhotosEditor::SessionPhotosEditor(SessionPhotosProcessor& p):AudioProcessorEditor(p),processor(p),list("Photos",this){
    look.setColour(ResizableWindow::backgroundColourId,bg);look.setColour(TextButton::buttonColourId,Colour(0xff2a3b43));look.setColour(TextButton::textColourOffId,ink);look.setColour(ComboBox::backgroundColourId,panel);look.setColour(ComboBox::textColourId,ink);look.setColour(PopupMenu::backgroundColourId,panel);look.setColour(PopupMenu::textColourId,ink);setLookAndFeel(&look);
    for(Component* c:std::initializer_list<Component*>{&list,&canvas,&title,&notes,&link,&network,&add,&remove,&rotate,&fit,&plus,&minus,&connect,&copy,&status,&networkHelp})addAndMakeVisible(c);
    for(auto* e:{&title,&notes,&link}){e->setColour(TextEditor::backgroundColourId,panel);e->setColour(TextEditor::textColourId,ink);e->setColour(TextEditor::outlineColourId,Colour(0xff33434b));e->setColour(TextEditor::focusedOutlineColourId,accent);e->setFont(FontOptions(15));}
    title.setTextToShowWhenEmpty("Nom de la photo",muted);notes.setMultiLine(true);notes.setReturnKeyStartsNewLine(true);notes.setTextToShowWhenEmpty(String::fromUTF8("Notes : appareil, patch, placement, réglages…"),muted);
    link.setReadOnly(true);link.setMultiLine(true);link.setFont(FontOptions(12));
    list.setRowHeight(78);list.setColour(ListBox::backgroundColourId,bg);list.setOutlineThickness(0);
    status.setColour(Label::textColourId,muted);status.setFont(FontOptions(13));
    networkHelp.setColour(Label::textColourId,muted);networkHelp.setFont(FontOptions(14));networkHelp.setJustificationType(Justification::topLeft);
    networkHelp.setText(String::fromUTF8("Téléphone sur le même réseau que le PC. Choisissez l’adresse Wi-Fi ou Ethernet du studio."),dontSendNotification);
    int id=1;for(auto ip:LocalServer::localAddresses())network.addItem(ip,id++);network.setSelectedId(1);network.setTextWhenNothingSelected(String::fromUTF8("Aucun réseau local détecté"));
    add.onClick=[this]{importFiles();};remove.onClick=[this]{if(auto p=selected()){
        auto safe=Component::SafePointer<SessionPhotosEditor>(this);
        AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,"Supprimer la photo ?",String::fromUTF8("Elle sera retirée de cette instance. L'original sur votre téléphone reste intact."),"Supprimer","Annuler",this,ModalCallbackFunction::create([safe,id=p->id](int result){if(safe && result){safe->processor.store.erase(id);safe->processor.modified();safe->refresh();}}));
    }};
    rotate.onClick=[this]{if(auto p=selected()){processor.store.edit(p->id,p->title,p->notes,p->rotation+1);processor.modified();refresh();}};
    fit.onClick=[this]{canvas.fit();};plus.onClick=[this]{canvas.zoomBy(1.3f);};minus.onClick=[this]{canvas.zoomBy(1/1.3f);};
    title.onTextChange=[this]{editSelected();};notes.onTextChange=[this]{editSelected();};title.setInputRestrictions(160);notes.setInputRestrictions(16000);
    connect.onClick=[this]{toggleServer();};copy.onClick=[this]{SystemClipboard::copyTextToClipboard(processor.server.url());};
    setResizable(true,true);setResizeLimits(1020,760,1800,1200);setSize(1140,820);refresh();
    if(processor.server.url().isNotEmpty()){link.setText(processor.server.url());qr=std::make_unique<qrcodegen::QrCode>(qrcodegen::QrCode::encodeText(processor.server.url().toRawUTF8(),qrcodegen::QrCode::Ecc::MEDIUM));connect.setButtonText("Fermer le transfert");network.setEnabled(false);}
    copy.setEnabled(processor.server.url().isNotEmpty());timerCallback();startTimerHz(4);
}
SessionPhotosEditor::~SessionPhotosEditor(){stopTimer();list.setModel(nullptr);setLookAndFeel(nullptr);}
std::shared_ptr<const Photo> SessionPhotosEditor::selected() const {for(auto& p:state.photos)if(p->id==selectedId)return p;return {};}
void SessionPhotosEditor::editSelected(){if(populating)return;if(auto p=selected()){if(processor.store.edit(p->id,title.getText(),notes.getText(),p->rotation)){processor.modified();state=processor.store.snapshot();list.repaint();}}}
void SessionPhotosEditor::showSelected(){populating=true;auto p=selected();title.setText(p?p->title:String(),false);notes.setText(p?p->notes:String(),false);title.setEnabled(bool(p));notes.setEnabled(bool(p));remove.setEnabled(bool(p));rotate.setEnabled(bool(p));canvas.setPhoto(p);populating=false;}
void SessionPhotosEditor::selectedRowsChanged(int row){selectedId=isPositiveAndBelow(row,state.photos.size())?state.photos[size_t(row)]->id:String();showSelected();}
void SessionPhotosEditor::refresh(){state=processor.store.snapshot();if(!selected() && !state.photos.empty())selectedId=state.photos.back()->id;list.updateContent();int row=-1;for(size_t i=0;i<state.photos.size();++i)if(state.photos[i]->id==selectedId)row=int(i);if(row<0)list.deselectAllRows();else list.selectRow(row);showSelected();repaint();}
void SessionPhotosEditor::timerCallback(){if(processor.store.snapshot().revision!=state.revision)refresh();status.setText(processor.restoreFailed?String::fromUTF8("ATTENTION : état illisible. Les photos precedentes n’ont pas été remplacées."):String(state.photos.size())+String::fromUTF8(" / 64 photos  ·  ")+String(double(state.bytes)/1048576,1)+String::fromUTF8(" / 64 Mo  ·  Sauvegardez le projet pour conserver vos modifications."),dontSendNotification);}
void SessionPhotosEditor::paintListBoxItem(int row,Graphics& g,int w,int h,bool active){if(!isPositiveAndBelow(row,state.photos.size()))return;auto p=state.photos[size_t(row)];if(active){g.setColour(Colour(0xff29443b));g.fillRoundedRectangle(4,3,float(w-8),float(h-6),8);}g.drawImageWithin(p->preview,12,12,64,h-24,RectanglePlacement::centred);g.setColour(active?accent:ink);g.setFont(14);g.drawFittedText(p->title.isEmpty()?"Sans titre":p->title,86,12,w-98,h-24,Justification::centredLeft,3);}
void SessionPhotosEditor::paint(Graphics& g){g.fillAll(bg);
    g.setColour(panel);g.fillRoundedRectangle(phoneArea.toFloat(),14);g.setColour(ink);g.setFont(FontOptions(19,Font::bold));g.drawText(String::fromUTF8("Depuis votre téléphone"),phoneArea.reduced(16).removeFromTop(28),Justification::left);
    if(qr){const int n=qr->getSize(),cell=jmax(1,jmin(qrArea.getWidth(),qrArea.getHeight())/(n+8)),side=(n+8)*cell;auto box=Rectangle<int>(side,side).withCentre(qrArea.getCentre());g.setColour(Colours::white);g.fillRect(box);g.setColour(Colours::black);for(int y=0;y<n;++y)for(int x=0;x<n;++x)if(qr->getModule(x,y))g.fillRect(box.getX()+(x+4)*cell,box.getY()+(y+4)*cell,cell,cell);
    }else{g.setColour(muted);g.setFont(15);g.drawFittedText("Activez le transfert\npour afficher le QR code.",qrArea,Justification::centred,3);}}
void SessionPhotosEditor::resized(){auto r=getLocalBounds().reduced(20);auto toolbar=r.removeFromTop(36);add.setBounds(toolbar.removeFromLeft(110));toolbar.removeFromLeft(10);remove.setBounds(toolbar.removeFromLeft(110));toolbar.removeFromLeft(20);rotate.setBounds(toolbar.removeFromLeft(90));toolbar.removeFromLeft(10);fit.setBounds(toolbar.removeFromLeft(90));toolbar.removeFromLeft(10);minus.setBounds(toolbar.removeFromLeft(36));toolbar.removeFromLeft(6);plus.setBounds(toolbar.removeFromLeft(36));r.removeFromTop(16);status.setBounds(r.removeFromBottom(30));r.removeFromBottom(8);phoneArea=r.removeFromRight(252);r.removeFromRight(16);list.setBounds(r.removeFromLeft(210));r.removeFromLeft(14);auto noteArea=r.removeFromBottom(140);title.setBounds(noteArea.removeFromTop(34));noteArea.removeFromTop(8);notes.setBounds(noteArea);r.removeFromBottom(12);canvas.setBounds(r);
    auto phone=phoneArea.reduced(16);phone.removeFromTop(32);networkHelp.setBounds(phone.removeFromTop(50));phone.removeFromTop(6);network.setBounds(phone.removeFromTop(30));phone.removeFromTop(8);connect.setBounds(phone.removeFromTop(34));phone.removeFromTop(10);auto bottom=phone.removeFromBottom(94);qrArea=phone;bottom.removeFromTop(8);link.setBounds(bottom.removeFromTop(50));bottom.removeFromTop(6);copy.setBounds(bottom.removeFromTop(30));}

void SessionPhotosEditor::toggleServer(){if(processor.server.url().isNotEmpty()){processor.server.stop();qr.reset();link.clear();connect.setButtonText("Activer le transfert");network.setEnabled(true);}else{
    auto error=processor.server.start(network.getText());if(error.isNotEmpty()){AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Connexion",error);return;}
    link.setText(processor.server.url());qr=std::make_unique<qrcodegen::QrCode>(qrcodegen::QrCode::encodeText(processor.server.url().toRawUTF8(),qrcodegen::QrCode::Ecc::MEDIUM));connect.setButtonText("Fermer le transfert");network.setEnabled(false);
}copy.setEnabled(qr!=nullptr);repaint();}
void SessionPhotosEditor::importFiles(){chooser=std::make_unique<FileChooser>("Importer des photos",File(),"*.jpg;*.jpeg;*.png");auto safe=Component::SafePointer<SessionPhotosEditor>(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::canSelectMultipleItems,[safe](const FileChooser& fc){if(!safe)return;StringArray errors;for(auto f:fc.getResults()){if(f.getSize()>int64(PhotoStore::maxPhotoBytes)){errors.add(f.getFileName()+" : limite 8 Mo.");continue;}MemoryBlock b;if(!f.loadFileAsData(b)){errors.add(f.getFileName()+" : lecture impossible.");continue;}String id;auto error=safe->processor.store.add(b.getData(),b.getSize(),f.getFileNameWithoutExtension(),&id);if(error.isNotEmpty())errors.add(error);else{safe->selectedId=id;safe->processor.modified();}}safe->refresh();if(!errors.isEmpty())AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Import",errors.joinIntoString("\n"));});}
