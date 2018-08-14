#include <iostream>
#include <string>
#include <iomanip>
#include <map>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include "message.h"
#include "order.h"

#ifndef BATS_PITCH_VOLUMECALCULATOR_H
#define BATS_PITCH_VOLUMECALCULATOR_H
namespace pitch {
namespace bats {

typedef std::map<std::string, std::unique_ptr<Order>> OrderCache;

template<int32_t N = 10>
class TopVolumeCalculator {

  static constexpr int SHARES_SIZE = 6;

 private:
  OrderCache order_cache_;
  std::map<std::string, int64_t> executed_volume_;

  void execute(const std::string &symbol, const uint32_t volume) {
    auto found_iter = executed_volume_.find(symbol);
    if (found_iter == executed_volume_.end()) {
      executed_volume_[symbol] = 0;
    }

    executed_volume_[symbol] += volume;
  }

  void cleanup(const std::string &order_id) {
    if (order_cache_[order_id]->quantity == 0) {
      order_cache_.erase(order_id);
    }
  }

 public:
  void process(std::istream &in) {
    uint32_t count = 1;

    while (in.good() && parse<char>(in, 1) /*Get First Character S and Ignore*/ ) {

      std::cout << "Processing " << count << " -> " << " .. ";

      PitchMessageheader header(in);

      if (header.message_type == MessageType::ADD_ORDER_SHORT) {
        AddOrderShortMessage message(header, in);
        order_cache_[message.order_id()] = std::unique_ptr<Order>(new Order);
        order_cache_[message.order_id()]->order_id = message.order_id();
        order_cache_[message.order_id()]->side = message.side();
        order_cache_[message.order_id()]->quantity = message.shares();
        order_cache_[message.order_id()]->symbol = message.symbol();
        order_cache_[message.order_id()]->price = message.price();
        std::cout << " .. ADDED SHORT";
      } else if (header.message_type == MessageType::ADD_ORDER_LONG) {
        AddOrderLongMessage message(header, in);
        order_cache_[message.order_id()] = std::unique_ptr<Order>(new Order);
        order_cache_[message.order_id()]->order_id = message.order_id();
        order_cache_[message.order_id()]->side = message.side();
        order_cache_[message.order_id()]->quantity = message.shares();
        order_cache_[message.order_id()]->symbol = message.symbol();
        order_cache_[message.order_id()]->price = message.price();
      } else if (header.message_type == MessageType::ORDER_EXECUTED) {
        OrderExecutionMessage message(header, in);
        order_cache_[message.order_id()]->quantity -= message.shares();
        execute(order_cache_[message.order_id()]->symbol, message.shares());
        cleanup(message.order_id());
        std::cout << " .. EXEC";
      } else if (header.message_type == MessageType::ORDER_CANCEL) {
        OrderCancelMessage message(header, in);
        order_cache_[message.order_id()]->quantity -= message.shares();
        cleanup(message.order_id());
        std::cout << " .. ORDER CANCEL";
      } else if (header.message_type == MessageType::TRADE_SHORT) {
        TradeShortMessage message(header, in);
        execute(message.symbol(), message.shares());
        std::cout << " .. TRADE SHORT";
      } else if (header.message_type == MessageType::TRADE_LONG) {
        TradeLongMessage message(header, in);
        execute(message.symbol(), message.shares());
        std::cout << " .. TRADE LONG";
      } else {
        std::string remaining;
        std::getline(in, remaining);
        std::cout << remaining;
      }

      std::cout << std::endl;

      count++;
    }

  }
  friend std::ostream &operator<<(std::ostream &out, const TopVolumeCalculator &c) {
//    typedef std::pair<std::string, int64_t > Pair;
//
//    std::array<int64_t , N> top_volume;
//
//    std::partial_sort_copy(
//        c.executed_volume_.cbegin(), c.executed_volume_.cend(),
//        top_volume.begin(), top_volume.end(),
//        [](const Pair &lhs, const Pair &rhs) {
//          return lhs.second > rhs.second;
//        }
//    );
//

    std::vector<std::pair<std::string, int64_t >> top_volume(c.executed_volume_.size());

    for (auto &ev: c.executed_volume_) {
      top_volume.push_back(ev);
    }

    std::partial_sort(top_volume.begin(),
                      top_volume.begin() + 10,
                      top_volume.end(),
                      [](std::pair<std::string, int64_t > &l,
                         std::pair<std::string, int64_t > &r) {
                        return l.second > r.second;
                      });

    for (auto &pair: top_volume) {
      out << std::left << std::setw(6) << pair.first << std::right << std::setw(10) << pair.second << "\n";
    }

    return out;
//    std::partial_sort_copy(c.executed_volume_.begin(),
//                           c.executed_volume_.end(),
//                           top_volume.begin(),
//                           top_volume.end(),
//                           [](std::pair<const std::string, int64_t > const& l,
//                              std::pair<const std::string, int64_t > const& r)
//                           {
//                             return l.second > r.second;
//                           });
  }
};

}
}
#endif //BATS_PITCH_VOLUMECALCULATOR_H
