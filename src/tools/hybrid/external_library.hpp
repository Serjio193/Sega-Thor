#pragma once

#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace oasis::hybrid {

class ExternalLibrary {
public:
    explicit ExternalLibrary(const char* path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(path);
#else
        handle_ = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
        if (!handle_) throw std::runtime_error("cannot load external GPGX library");
    }
    template<class T> T get(const char* name) {
#ifdef _WIN32
        const auto symbol = GetProcAddress(handle_, name);
#else
        const auto symbol = dlsym(handle_, name);
#endif
        if (!symbol) throw std::runtime_error(std::string("missing GPGX symbol ") + name);
        return reinterpret_cast<T>(symbol);
    }

private:
#ifdef _WIN32
    HMODULE handle_{};
#else
    void* handle_{};
#endif
};

} // namespace oasis::hybrid
