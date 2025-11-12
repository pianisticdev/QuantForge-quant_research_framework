//
// Created by Daniel Griffiths on 11/5/25.
//

#include "plugin_api.h"
#include <vector>
#include <string>
#include <cstring>

struct SMAState {
    int fast = 10, slow = 30;
    std::string symbol;
    std::vector<double> closes;
    bool long_on = false;
    double entry_px = 0.0;
    double pnl = 0.0;
};

static double sma(const std::vector<double>& v, int w) {
    if ((int)v.size() < w) return 0.0;
    double s=0.0; for (int i=v.size()-w;i<(int)v.size();++i) s+=v[i];
    return s / w;
}

static PluginResult init_fn(void* self, const PluginOptions* o) {
    auto& S = *static_cast<SMAState*>(self);
    if (o) {
        for (size_t i=0;i<o->count;++i) {
            auto k=o->items[i].key, v=o->items[i].value;
            if (strcmp(k,"fast")==0) S.fast = std::atoi(v);
            else if (strcmp(k,"slow")==0) S.slow = std::atoi(v);
            else if (strcmp(k,"symbol")==0) S.symbol = v;
        }
    }
    return {0,nullptr};
}

static PluginResult prep_fn(void* /*self*/, const char*, int64_t, int64_t, int32_t) {
    return PluginResult{0,nullptr};
}

static PluginResult on_bar_fn(void* self, const Bar* b) {
    auto& S = *static_cast<SMAState*>(self);
    S.closes.push_back(b->close);
    double f = sma(S.closes, S.fast);
    double s = sma(S.closes, S.slow);
    if (f==0 || s==0) return PluginResult{0,nullptr};

    // simple crossover: go long on golden cross, flat on death cross
    if (!S.long_on && f > s) { S.long_on = true; S.entry_px = b->close; }
    else if (S.long_on && f < s) {
        S.pnl += (b->close - S.entry_px);
        S.long_on = false;
    }
    return PluginResult{0,nullptr};
}

static PluginResult on_tick_fn(void*, const Tick*) { return PluginResult{0,nullptr}; }

static PluginResult finalize_fn(void* self, const char** out) {
    auto& S = *static_cast<SMAState*>(self);
    if (S.long_on && !S.closes.empty()) {
        S.pnl += (S.closes.back() - S.entry_px);
        S.long_on = false;
    }
    std::string json = std::string("{\"symbol\":\"")+S.symbol+
                       "\",\"pnl\":"+std::to_string(S.pnl)+
                       ",\"fast\":"+std::to_string(S.fast)+
                       ",\"slow\":"+std::to_string(S.slow)+"}";
    char* heap = new char[json.size()+1];
    std::memcpy(heap, json.c_str(), json.size()+1);
    *out = heap;
    return PluginResult{0,nullptr};
}

static void free_str_fn(void*, const char* p){ delete[] p; }
static void destroy_fn(void* self){ delete static_cast<SMAState*>(self); }

extern "C" PluginExport create_plugin(const PluginContext* /*ctx*/) {
    auto* S = new SMAState{};
    PluginVTable vt{
            &destroy_fn,
            &init_fn,
            &prep_fn,
            &on_bar_fn,
            &on_tick_fn,
            &finalize_fn,
            &free_str_fn
    };
    return PluginExport{ PLUGIN_API_VERSION, S, vt };
}
