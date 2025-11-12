#ifndef QUANT_FORGE_CONSOLE_RENDERER_HPP
#define QUANT_FORGE_CONSOLE_RENDERER_HPP

#pragma once

#include <vector>

#include "../simulators/back_test/back_test_engine.hpp"
#include "../simulators/monte_carlo/monte_carlo_engine.hpp"
#include "interface.hpp"

namespace renderers {

    class ConsoleRenderer : public IRenderer {
       public:
        void render_back_test_report(const std::vector<simulators::BackTestReport>& reports) override;
        void render_monte_carlo_report(const std::vector<simulators::MonteCarloReport>& reports) override;
    };
}  // namespace renderers

#endif
