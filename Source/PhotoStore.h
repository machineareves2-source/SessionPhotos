#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <mutex>
#include <memory>
#include <vector>

struct Photo {
    juce::String id, title, notes;
    int rotation = 0;
    juce::MemoryBlock bytes;
    juce::Image preview;
};
class PhotoStore {
public:
    static constexpr size_t maxPhotoBytes = 8 * 1024 * 1024;
    static constexpr size_t maxTotalBytes = 64 * 1024 * 1024;
    static constexpr size_t maxPhotos = 64;
    struct Snapshot { std::vector<std::shared_ptr<const Photo>> photos; uint64_t revision; size_t bytes; };
    Snapshot snapshot() const;
    juce::String add(const void*, size_t, const juce::String&, juce::String* newId = nullptr);
    bool edit(const juce::String&, const juce::String&, const juce::String&, int);
    bool erase(const juce::String&);
    void save(juce::MemoryBlock&) const;
    bool restore(const void*, size_t);
    static bool imageDimensions(const void*, size_t, int&, int&);
private:
    mutable std::mutex mutex;
    std::vector<std::shared_ptr<const Photo>> photos;
    uint64_t revision = 0;
    size_t totalBytes = 0;
};
