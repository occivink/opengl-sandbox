#include "fetch.hpp"

#include <esmcripten/fetch.h>

/*void fetchImageSuccess(emscripten_fetch_t *fetch)
{
    auto c = on_scope_end([&]{ emscripten_fetch_close(fetch); });
    if (fetch->numBytes == 0) {
        Log::Error("expected data, but received none");
        return;
    }
    Log::Info("trying to load received data (" + std::to_string(fetch->numBytes/1000) + "kb) as image");
    if (loadImageToQuad(reinterpret_cast<const unsigned char*>(fetch->data), fetch->numBytes)) {
        current_image_data.resize(fetch->numBytes);
        std::memcpy(current_image_data.data(), fetch->data, current_image_data.size());
    }
}

void getProcessingResultSuccess(emscripten_fetch_t *fetch)
{
    auto c = on_scope_end([&]{ emscripten_fetch_close(fetch); });
    if (fetch->numBytes == 0) {
        Log::Error("expected data, but received none");
        return;
    }
    Log::Info("trying to load received data (" + std::to_string(fetch->numBytes/1000) + "kb) as image");
    if (loadImageToQuad(reinterpret_cast<const unsigned char*>(fetch->data), fetch->numBytes)) {
        current_image_data.resize(fetch->numBytes);
        std::memcpy(current_image_data.data(), fetch->data, current_image_data.size());
        resetLabels();
    }
}

void genericSuccess(emscripten_fetch_t *fetch)
{
    Log::Info("api call succeeded");
    emscripten_fetch_close(fetch);
}

void genericFail(emscripten_fetch_t *fetch)
{
    Log::Error("api call failed");
    emscripten_fetch_close(fetch);
}

void sendSerializedImage(const char* apiPath, const std::vector<unsigned char>& png_data)
{
    Log::Info("sending image to "s + apiPath);
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = genericSuccess;
    attr.onerror = genericFail;
    int n = png_data.size();
    auto tmp = new char[n];
    std::memcpy(tmp, png_data.data(), n);
    attr.requestDataSize = n;
    attr.requestData = tmp;
    auto s = "api/"s + apiPath;
    emscripten_fetch(&attr, s.c_str());
}

void getProcessingResult()
{
    Log::Info("waiting for processing result");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = getProcessingResultSuccess;
    attr.onerror = genericFail;
    emscripten_fetch(&attr, "api/ProcessImages");
}

void fetchRandomImage()
{
    Log::Info("fetching random image");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = fetchImageSuccess;
    attr.onerror = genericFail;
    emscripten_fetch(&attr, "api/RandomImage");
}
*/
