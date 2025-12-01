#ifndef QUANT_FORGE_MODELS_MODELS_HPP
#define QUANT_FORGE_MODELS_MODELS_HPP

#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "../../utils/money_utils.hpp"
#include "../plugins/abi/abi.h"

using namespace money_utils;

namespace models {
    constexpr int64_t NULL_MARKET_TRIGGER_PRICE = INT64_MIN;

    struct Instruction {
        std::string symbol_;
        std::string action_;
        double quantity_;
        int64_t created_at_ns_ = std::chrono::system_clock::now().time_since_epoch().count();
        int64_t filled_at_ns_ = 0;

        std::optional<std::string> order_type_;  // Market / Limit /
        std::optional<Money> limit_price_;
        std::optional<Money> stop_loss_price_;
        std::optional<Money> take_profit_price_;

        explicit Instruction(const CInstruction& inst)
            : symbol_(inst.symbol_ != nullptr ? inst.symbol_ : ""),
              action_(inst.action_ != nullptr ? inst.action_ : ""),
              quantity_(inst.quantity_),
              order_type_(inst.order_type_ != nullptr ? inst.order_type_ : ""),
              limit_price_(inst.limit_price_),
              stop_loss_price_(inst.stop_loss_price_ != NULL_MARKET_TRIGGER_PRICE ? std::make_optional(inst.stop_loss_price_) : std::nullopt),
              take_profit_price_(inst.take_profit_price_ != NULL_MARKET_TRIGGER_PRICE ? std::make_optional(inst.take_profit_price_) : std::nullopt) {}
        Instruction(std::string symbol, double quantity, std::string action) : symbol_(std::move(symbol)), action_(std::move(action)), quantity_(quantity) {}

        [[nodiscard]] static std::vector<Instruction> to_instructions(const PluginResult& result) {
            std::vector<Instruction> vec;
            vec.reserve(result.instructions_count_);
            for (size_t i = 0; i < result.instructions_count_; ++i) {
                vec.emplace_back(result.instructions_[i]);
            }
            return vec;
        }

        [[nodiscard]] bool is_signal() const { return !this->order_type_.has_value(); }
        [[nodiscard]] bool is_order() const { return this->order_type_.has_value(); }
        [[nodiscard]] bool is_limit_order() const { return this->order_type_.has_value() && this->order_type_.value() == constants::LIMIT; }
        [[nodiscard]] bool is_stop_loss_order() const { return this->order_type_.has_value() && this->order_type_.value() == constants::STOP_LOSS; }
        [[nodiscard]] bool is_buy() const { return this->action_ == constants::BUY; }
        [[nodiscard]] bool is_sell() const { return this->action_ == constants::SELL; }

        bool operator<(const Instruction& other) const { return filled_at_ns_ < other.filled_at_ns_; }
    };

    struct Position {
        std::string symbol_;
        double quantity_;
        Money average_price_;

        [[nodiscard]] static std::vector<CPosition> to_c_positions(const std::map<std::string, Position>& positions) {
            std::vector<CPosition> c_positions;
            c_positions.reserve(positions.size());
            for (const auto& [symbol, position] : positions) {
                c_positions.emplace_back(CPosition{
                    .symbol_ = symbol.c_str(),
                    .quantity_ = position.quantity_,
                    .average_price_ = position.average_price_.to_abi_double(),
                });
            }
            return c_positions;
        }
    };

    struct Trade {
        bool is_market_sell_triggered_ = false;
        double quantity_;
        Money price_;
        int64_t timestamp_ns_;
        std::string symbol_;
        std::optional<Money> stop_loss_price_;
        std::optional<Money> take_profit_price_;

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        Trade(std::string symbol, double quantity, Money price, int64_t timestamp_ns, std::optional<Money> stop_loss_price,
              std::optional<Money> take_profit_price)
            : quantity_(quantity),
              price_(price),
              timestamp_ns_(timestamp_ns),
              symbol_(std::move(symbol)),
              stop_loss_price_(stop_loss_price),
              take_profit_price_(take_profit_price) {}

        [[nodiscard]] static std::vector<CTrade> to_c_trades(const std::vector<Trade>& trades) {
            std::vector<CTrade> c_trades;
            c_trades.reserve(trades.size());
            for (const auto& trade : trades) {
                c_trades.emplace_back(CTrade{
                    .symbol_ = trade.symbol_.c_str(),
                    .quantity_ = trade.quantity_,
                    .price_ = trade.price_.to_abi_int64(),
                    .timestamp_ns_ = trade.timestamp_ns_,
                    .stop_loss_price_ = trade.stop_loss_price_.value_or(Money(NULL_MARKET_TRIGGER_PRICE)).to_abi_int64(),
                    .take_profit_price_ = trade.take_profit_price_.value_or(Money(NULL_MARKET_TRIGGER_PRICE)).to_abi_int64(),
                    .is_market_sell_triggered_ = trade.is_market_sell_triggered_,
                });
            }
            return c_trades;
        }

        [[nodiscard]] Instruction to_sell_instruction() const { return {this->symbol_, this->quantity_, constants::SELL}; }
    };

    // We calculate the rolling windows once per day
    // We need to track peak equity in memory
    // We should track average return across rolling windows in memory
    struct EquitySnapshot {
        int64_t timestamp_ns_;
        Money equity_;
        double return_;
        double max_drawdown_;
        double sharpe_ratio_;
        double sharpe_ratio_rolling_;
        double sortino_ratio_;
        double sortino_ratio_rolling_;
        double calmar_ratio_;
        double calmar_ratio_rolling_;
        double tail_ratio_;
        double tail_ratio_rolling_;
        double value_at_risk_;
        double value_at_risk_rolling_;
        double conditional_value_at_risk_;
        double conditional_value_at_risk_rolling_;

        [[nodiscard]] static std::vector<CEquitySnapshot> to_c_equity_snapshots(const std::vector<EquitySnapshot>& equity_snapshots) {
            std::vector<CEquitySnapshot> c_equity_snapshots;
            c_equity_snapshots.reserve(equity_snapshots.size());
            for (const auto& equity_snapshot : equity_snapshots) {
                c_equity_snapshots.emplace_back(CEquitySnapshot{
                    .timestamp_ns_ = equity_snapshot.timestamp_ns_,
                    .equity_ = equity_snapshot.equity_.to_abi_int64(),
                    .return_ = equity_snapshot.return_,
                    .max_drawdown_ = equity_snapshot.max_drawdown_,
                    .sharpe_ratio_ = equity_snapshot.sharpe_ratio_,
                    .sortino_ratio_ = equity_snapshot.sortino_ratio_,
                    .calmar_ratio_ = equity_snapshot.calmar_ratio_,
                    .tail_ratio_ = equity_snapshot.tail_ratio_,
                    .value_at_risk_ = equity_snapshot.value_at_risk_,
                    .conditional_value_at_risk_ = equity_snapshot.conditional_value_at_risk_,
                });
            }
            return c_equity_snapshots;
        }
    };

    struct BackTestState {
        Money cash_;
        std::map<std::string, Position> positions_;
        std::map<std::string, Money> current_prices_;
        std::map<std::string, int64_t> current_volumes_;
        int64_t current_timestamp_ns_;
        std::vector<Trade> trade_history_;
        std::vector<EquitySnapshot> equity_curve_;

        mutable std::vector<CPosition> c_positions_cache_;
        mutable std::vector<CTrade> c_trades_cache_;
        mutable std::vector<CEquitySnapshot> c_equity_cache_;

        [[nodiscard]] static CState to_c_state(BackTestState& state) {
            state.c_positions_cache_ = Position::to_c_positions(state.positions_);
            state.c_trades_cache_ = Trade::to_c_trades(state.trade_history_);
            state.c_equity_cache_ = EquitySnapshot::to_c_equity_snapshots(state.equity_curve_);

            return CState{.cash_ = state.cash_.to_abi_int64(),
                          .positions_ = state.c_positions_cache_.data(),
                          .positions_count_ = state.c_positions_cache_.size(),
                          .trade_history_ = state.c_trades_cache_.data(),
                          .trade_history_count_ = state.c_trades_cache_.size(),
                          .equity_curve_ = state.c_equity_cache_.data(),
                          .equity_curve_count_ = state.c_equity_cache_.size()};
        }
    };
}  // namespace models

#endif
