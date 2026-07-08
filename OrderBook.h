#pragma once
#include "OrderBookEntry.h"
#include "CSVReader.h"
#include "Candlestick.h"
#include <string>
#include <vector>
#include <functional>

class OrderBook
{
    public:
        /** Given dataset only contains year 2020; Task 4 anchors drift/backfill to this */
        static constexpr int DATASET_YEAR = 2020;

    /** construct, reading a csv data file */
        OrderBook(std::string filename);
    /** return vector of all know products in the dataset*/
        std::vector<std::string> getKnownProducts();
    /** return vector of Orders according to the sent filters*/
        std::vector<OrderBookEntry> getOrders(OrderBookType type,
                                              std::string product,
                                              std::string timestamp);

        /** Task 1: orders of type/product whose date falls in [startDate, endDate] inclusive.
         *  Dates are "YYYY/MM/DD"; empty string = no bound on that side. */
        std::vector<OrderBookEntry> getOrdersInRange(OrderBookType type,
                                                     const std::string& product,
                                                     const std::string& startDate,
                                                     const std::string& endDate);

        /** Task 1: OHLC candles for a product+type, grouped by timestamp prefix length:
         *  groupByChars 4 = yearly ("2020"), 10 = daily ("2020/06/01").
         *  Single pass over the filtered orders — no per-group re-scans. */
        std::vector<Candlestick> computeOHLC(OrderBookType type,
                                             const std::string& product,
                                             const std::string& startDate,
                                             const std::string& endDate,
                                             int groupByChars);

        /** returns the earliest time in the orderbook*/
        std::string getEarliestTime();
        /** returns the next time after the
         * sent time in the orderbook
         * If there is no next timestamp, wraps around to the start
         * */
        std::string getNextTime(std::string timestamp);

        void insertOrder(OrderBookEntry& order);

        std::vector<OrderBookEntry> matchAsksToBids(std::string product, std::string timestamp);

        /** Task 4: last real price for a product/type, ignoring anything simulated —
         *  only entries with the original dataset username count as a seed. */
        double getSeedPrice(OrderBookType type, const std::string& product);

    private:
        /** Filter logic core function, both getOrders variants delegate here */
        std::vector<OrderBookEntry> filterOrders(const std::function<bool(const OrderBookEntry&)>& pred);

        std::vector<OrderBookEntry> orders;
};
