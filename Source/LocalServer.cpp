#include <httplib.h>
#include "LocalServer.h"
#include <BinaryData.h>
#include <random>
using namespace juce;
LocalServer::LocalServer(PhotoStore& s,std::function<void()> cb):store(s),changed(std::move(cb)){}
LocalServer::~LocalServer(){stop();}
StringArray LocalServer::localAddresses() {
    Array<IPAddress> ips; IPAddress::findAllAddresses(ips,false); StringArray result;
    for(auto ip:ips) { auto s=ip.toString(); auto parts=StringArray::fromTokens(s,".","");
        if(parts.size()!=4) continue;
        int a=parts[0].getIntValue(),b=parts[1].getIntValue();
        if(a==10 || (a==192 && b==168) || (a==172 && b>=16 && b<=31)) result.addIfNotAlreadyThere(s);
    }
    return result;
}
String LocalServer::start(const String& address) {
    stop();
    // Loopback is permitted for local integration tests; UI only offers private LAN addresses.
    if(address!="127.0.0.1" && !localAddresses().contains(address)) return "Choisir une adresse du reseau local.";
    std::random_device rng; String token;
    for(int i=0;i<8;++i) token+=String::toHexString(uint32_t(rng())).paddedLeft('0',8);
    const auto root=("/"+token).toStdString();
    server=std::make_unique<httplib::Server>();
    server->new_task_queue=[] { return new httplib::ThreadPool(2,4); };
    server->set_payload_max_length(PhotoStore::maxPhotoBytes);
    server->set_read_timeout(10,0); server->set_write_timeout(10,0); server->set_keep_alive_max_count(4);
    server->set_default_headers({{"Cache-Control","no-store"},{"X-Content-Type-Options","nosniff"},{"Referrer-Policy","no-referrer"},{"Content-Security-Policy","default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src 'self' blob: data:; connect-src 'self'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'"}});
    const int port=server->bind_to_any_port(address.toStdString());
    if(port<=0) {server.reset();return "Impossible d'ouvrir le port local.";}
    const auto host=(address+":"+String(port)).toStdString();
    const auto origin="http://"+host;
    server->set_pre_routing_handler([host,origin](const httplib::Request& req,httplib::Response& res) {
        if(req.get_header_value("Host")!=host || (req.has_header("Origin") && req.get_header_value("Origin")!=origin)) {
            res.status=403; return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
    server->Get(root+"/?",[](const auto&,auto& res){res.set_content(BinaryData::index_html,BinaryData::index_htmlSize,"text/html; charset=utf-8");});
    server->Get(root+"/api/photos",[this](const auto&,auto& res) {
        auto s=store.snapshot(); Array<var> photos;
        for(auto& p:s.photos) { auto* o=new DynamicObject(); o->setProperty("id",p->id);o->setProperty("title",p->title);o->setProperty("notes",p->notes);o->setProperty("rotation",p->rotation); photos.add(var(o)); }
        auto* o=new DynamicObject();o->setProperty("photos",var(photos));o->setProperty("bytes",int64(s.bytes));o->setProperty("revision",int64(s.revision));
        res.set_content(JSON::toString(var(o)).toStdString(),"application/json");
    });
    server->Post(root+"/api/upload",[this](const auto& req,auto& res) {
        String id; auto name=String::fromUTF8(req.get_param_value("name").c_str());
        auto error=store.add(req.body.data(),req.body.size(),name,&id);
        if(error.isNotEmpty()){res.status=422;res.set_content(error.toStdString(),"text/plain; charset=utf-8");return;}
        changed(); res.status=201;res.set_content(id.toStdString(),"text/plain");
    });
    server->Get(root+"/api/image/([a-zA-Z0-9-]+)",[this](const auto& req,auto& res) {
        auto id=String(req.matches[1].str());
        for(auto& p:store.snapshot().photos) if(p->id==id) {
            auto b=static_cast<const char*>(p->bytes.getData());
            res.set_content(b,p->bytes.getSize(),uint8_t(b[0])==137?"image/png":"image/jpeg");return;
        }
        res.status=404;
    });
    currentUrl=String(origin+root+"/");
    worker=std::thread([this]{server->listen_after_bind();});
    server->wait_until_ready();
    return {};
}
void LocalServer::stop(){if(server){server->stop();if(worker.joinable())worker.join();server.reset();}currentUrl.clear();}
