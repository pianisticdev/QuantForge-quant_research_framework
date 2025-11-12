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
    LibHandle handle_{};

    static LibHandler open(const std::string& path, std::string& err) {
        LibHandler l{};
#if defined(_WIN32)
        l.handle_ = LoadLibraryA(path.c_str());
        if (l.handle_ == nullptr) {
            err = "LoadLibrary failed";
        }
#else
        l.handle_ = dlopen(path.c_str(), RTLD_NOW);
        if (l.handle_ == nullptr) {
            err = dlerror();
        }
#endif
        return l;
    }

    template <class Fn>
    Fn sym(const char* name, std::string& err) const {
#if defined(_WIN32)
        auto* p = GetProcAddress(handle_, name);
        if (p == nullptr) {
            err = "GetProcAddress failed";
            return nullptr;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<Fn>(p);
#else
        auto* p = dlsym(handle_, name);
        if (p == nullptr) {
            err = dlerror();
            return nullptr;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<Fn>(p);
#endif
    }

    void close() {
#if defined(_WIN32)
        if (handle_ != nullptr) {
            FreeLibrary(handle_);
        }
#else
        if (handle_ != nullptr) {
            dlclose(handle_);
        }
#endif
        handle_ = nullptr;
    }
};

#endif
