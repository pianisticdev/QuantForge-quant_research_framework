#include "console_renderer.hpp"

#include <iostream>
#include <vector>

#include "../simulators/back_test/back_test_engine.hpp"
#include "../simulators/monte_carlo/monte_carlo_engine.hpp"

namespace renderers {
    ConsoleRenderer::~ConsoleRenderer() = default;

    void ConsoleRenderer::render_back_test_report(const std::vector<simulators::BackTestReport>& reports) {
        std::cout << "----- Back Test Reports ----- " << std::endl;

        for (const auto& report : reports) {  // NOLINT
            std::cout << "TODO: Render back test report" << std::endl;
        }
    }

    void ConsoleRenderer::render_monte_carlo_report(const std::vector<simulators::MonteCarloReport>& reports) {
        std::cout << "----- Monte Carlo Reports ----- " << std::endl;

        for (const auto& report : reports) {  // NOLINT
            std::cout << "TODO: Render monte carlo report" << std::endl;
        }
    }
}  // namespace renderers
