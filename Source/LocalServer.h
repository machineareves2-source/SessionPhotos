#pragma once
#include "PhotoStore.h"
#include <functional>
#include <thread>
namespace httplib { class Server; }
class LocalServer {
public:
    LocalServer(PhotoStore&, std::function<void()>);
    ~LocalServer();
    juce::String start(const juce::String& address);
    void stop();
    juce::String url() const { return currentUrl; }
    static juce::StringArray localAddresses();
private:
    PhotoStore& store;
    std::function<void()> changed;
    std::unique_ptr<httplib::Server> server;
    std::thread worker;
    juce::String currentUrl;
};
