#include <httplib.h>
#include "LocalServer.h"
#include "PluginProcessor.h"
#include <iostream>
#include <atomic>
#include <thread>
using namespace juce;
static void check(bool condition,const char* label){if(!condition){std::cerr<<"FAIL: "<<label<<std::endl;std::exit(1);}std::cout<<"PASS: "<<label<<std::endl;}
int main(int argc,char** argv){
    if(argc>1 && std::string(argv[1])=="--serve") {
        PhotoStore store, secondStore;LocalServer server(store,[]{}), second(secondStore,[]{});
        if(second.start("127.0.0.1").isNotEmpty())return 1;
        auto error=server.start("127.0.0.1");if(error.isNotEmpty())return 1;
        std::cout<<server.url()<<std::endl;
        std::string line;std::getline(std::cin,line);return 0;
    }
    if(argc>1 && std::string(argv[1])=="--screenshot") {
        ScopedJuceInitialiser_GUI gui;
        SessionPhotosProcessor processor;
        processor.server.start("127.0.0.1");
        if(argc>3) {MemoryBlock b;File(String(argv[3])).loadFileAsData(b);processor.store.add(b.getData(),b.getSize(),"Reglages de session");}
        std::unique_ptr<AudioProcessorEditor> editor(processor.createEditor());
        editor->setVisible(true);
        auto img=editor->createComponentSnapshot(editor->getLocalBounds());
        File out{String(argv[2])};out.deleteFile();FileOutputStream stream{out};PNGImageFormat().writeImageToStream(img,stream);
        return 0;
    }

    Image image(Image::RGB,48,32,true);{Graphics g(image);g.fillAll(Colours::orange);}
    MemoryOutputStream png;PNGImageFormat().writeImageToStream(image,png);
    PhotoStore a;String id;
    check(a.add(png.getData(),png.getDataSize(),"Distressor voix",&id).isEmpty(),"PNG accepted");
    check(a.edit(id,"Voix",String::fromUTF8("Ratio 4:1 / micro à 20 cm"),3),"edit notes and rotation");
    MemoryBlock state;a.save(state);PhotoStore b;
    check(b.restore(state.getData(),state.getSize()),"restore state");auto s=b.snapshot();
    check(s.photos.size()==1 && s.photos[0]->id==id && s.photos[0]->notes==String::fromUTF8("Ratio 4:1 / micro à 20 cm") && s.photos[0]->rotation==3 && s.photos[0]->bytes==MemoryBlock(png.getData(),png.getDataSize()),"embedded bytes, unicode notes, ids and rotation survive round trip");
    check(!b.restore(state.getData(),state.getSize()-1) && b.snapshot().photos.size()==1,"truncated state rejected without loss");
    check(a.add("not an image",12,"bad").isNotEmpty(),"invalid upload rejected");
    auto bomb=MemoryBlock(png.getData(),png.getDataSize());auto* bytes=static_cast<unsigned char*>(bomb.getData());bytes[16]=0x7f;
    check(a.add(bomb.getData(),bomb.getSize(),"bomb").isNotEmpty(),"oversized dimensions rejected before decode");
    PhotoStore limits;for(size_t i=0;i<PhotoStore::maxPhotos;++i)check(limits.add(png.getData(),png.getDataSize(),"x").isEmpty(),"within count limit");
    check(limits.add(png.getData(),png.getDataSize(),"x").isNotEmpty(),"photo count limit");
    std::atomic<int> changes{0};LocalServer server(a,[&]{++changes;});
    check(server.start("127.0.0.1").isEmpty(),"start loopback receiver");
    URL url(server.url());httplib::Client client("127.0.0.1",url.getPort());client.set_connection_timeout(2);client.set_read_timeout(2);
    auto path="/"+url.getSubPath().toStdString();if(path.back()=='/')path.pop_back();
    httplib::Result page;
    for(int n=0;n<30;++n){page=client.Get(path+"/");if(page)break;std::this_thread::sleep_for(std::chrono::milliseconds(10));}
    check(page && page->status==200 && page->body.find("Prendre une photo")!=std::string::npos,"phone page served");
    auto unauthorized=client.Get("/wrong/api/photos");check(unauthorized && unauthorized->status==404,"unknown token rejected");
    auto crossOrigin=client.Get(path+"/api/photos",{{"Origin","https://example.com"}});check(crossOrigin && crossOrigin->status==403,"foreign browser origin rejected");
    auto upload=client.Post(path+"/api/upload?name=Test",std::string(static_cast<const char*>(png.getData()),png.getDataSize()),"image/png");
    check(upload && upload->status==201 && a.snapshot().photos.size()==2 && changes==1,"HTTP upload reaches store before acknowledgement");
    auto bad=client.Post(path+"/api/upload","bad","image/jpeg");check(bad && bad->status==422 && a.snapshot().photos.size()==2,"invalid HTTP upload does not mutate store");
    auto gallery=client.Get(path+"/api/photos");check(gallery && JSON::parse(gallery->body)["photos"].size()==2,"phone gallery lists photos");
    auto download=client.Get(path+"/api/image/"+id.toStdString());check(download && download->status==200 && download->body==std::string(static_cast<const char*>(png.getData()),png.getDataSize()),"image fetched byte-identically");
    MemoryBlock remoteState;a.save(remoteState);PhotoStore reopened;check(reopened.restore(remoteState.getData(),remoteState.getSize()) && reopened.snapshot().photos.size()==2,"received photo survives save/reopen without external files");
    PhotoStore otherStore; LocalServer other(otherStore,[]{});
    check(other.start("127.0.0.1").isEmpty(),"second receiver starts");
    auto instances=client.Get(path+"/api/instances");
    check(instances && JSON::parse(instances->body).size()==2,"both active instances discoverable");
    auto hidden=client.Get("/wrong/api/instances");
    check(hidden && hidden->status==404,"instance URLs require valid token");
    URL otherUrl(other.url()); httplib::Client otherClient("127.0.0.1",otherUrl.getPort());
    auto otherPath="/"+otherUrl.getSubPath().toStdString()+"api/upload?name=Other";
    auto otherUpload=otherClient.Post(otherPath,std::string(static_cast<const char*>(png.getData()),png.getDataSize()),"image/png");
    check(otherUpload && otherUpload->status==201 && otherStore.snapshot().photos.size()==1 && a.snapshot().photos.size()==2,"selected receiver keeps photos isolated");
    other.stop(); instances=client.Get(path+"/api/instances");
    check(instances && JSON::parse(instances->body).size()==1,"stopped instance removed from chooser");
    auto old=server.url();server.stop();check(server.start("127.0.0.1").isEmpty() && server.url()!=old,"new connection token on restart");server.stop();
    check(b.erase(id) && b.snapshot().photos.empty(),"delete photo");MemoryBlock empty;b.save(empty);check(reopened.restore(empty.getData(),empty.getSize()) && reopened.snapshot().photos.empty(),"empty state clears prior photos");
    std::cout<<"All state and local network tests passed.\n";
}
