//
// Created by Daniel Griffiths on 11/6/25.
//

#ifndef QUANT_FORGE_LIB_HANDLER_HPP
#define QUANT_FORGE_LIB_HANDLER_HPP

#pragma once
#include <string>
#if defined(_WIN32)
#include <windows.h>
  using LibHandle = HMODULE;
#else
#include <dlfcn.h>
using LibHandle = void*;
#endif

struct LibHandler {
    LibHandle handle{};

    static LibHandler open(const std::string& path, std::string& err) {
        LibHandler l{};
#if defined(_WIN32)
        l.handle = LoadLibraryA(path.c_str());
        if (!l.handle) err = "LoadLibrary failed";
#else
        l.handle = dlopen(path.c_str(), RTLD_NOW);
        if (!l.handle) err = dlerror();
#endif
        return l;
    }

    template<class Fn>
    Fn sym(const char* name, std::string& err) const {
#if defined(_WIN32)
        auto p = GetProcAddress(handle, name);
        if (!p) { err = "GetProcAddress failed"; return nullptr; }
        return reinterpret_cast<Fn>(p);
#else
        auto p = dlsym(handle, name);
        if (!p) { err = dlerror(); return nullptr; }
        return reinterpret_cast<Fn>(p);
#endif
    }

    void close() {
#if defined(_WIN32)
        if (handle) FreeLibrary(handle);
#else
        if (handle) dlclose(handle);
#endif
        handle = nullptr;
    }

};


#endif
