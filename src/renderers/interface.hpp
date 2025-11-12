#ifndef QUANT_FORGE_RENDERERS_INTERFACE_HPP
#define QUANT_FORGE_RENDERERS_INTERFACE_HPP

#pragma once

#include <vector>

#include "../simulators/back_test/back_test_engine.hpp"
#include "../simulators/monte_carlo/monte_carlo_engine.hpp"

namespace renderers {
    class IRenderer {
       public:
        IRenderer() = default;

        virtual ~IRenderer() = default;
        IRenderer(const IRenderer&) = delete;
        IRenderer& operator=(const IRenderer&) = delete;
        IRenderer(IRenderer&&) = delete;
        IRenderer& operator=(IRenderer&&) = delete;

        virtual void render_back_test_report(const std::vector<simulators::BackTestReport>& reports) = 0;
        virtual void render_monte_carlo_report(const std::vector<simulators::MonteCarloReport>& reports) = 0;
    };
}  // namespace renderers

#endif
