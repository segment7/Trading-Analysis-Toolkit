#pragma once
#include <string>
#include <vector>
#include <utility>
#include "OrderBookEntry.h"
#include "OrderBook.h"
#include "Wallet.h"

/** Task 3: owns USERS_TRADING.CSV — logs completed trades, shows history & stats.
 *  Row format: username,timestamp,product,orderType,price,amount ("ask" or "bid") */
class TransactionManager
{
    public:
        TransactionManager();

        /** append one completed trade for a user (call right after the wallet is updated) */
        void logTransaction(const std::string& username, const OrderBookEntry& sale);

        /** print the last n transactions for this user (any product), most recent first */
        void printRecent(const std::string& username, int n = 5);

        /** print ask/bid counts (all products + per product) and total spent in [startDate,endDate] */
        void printActivityStats(const std::string& username,
                                const std::string& startDate,
                                const std::string& endDate);

        /** Task 4: generate ordersPerSide asks + bids per product for a year, priced from
         *  a seed+drift+noise model, settled against the wallet and logged.
         *  Returns {filled, skipped}. */
        std::pair<int, int> generateTransactions(const std::string& username,
                                                 OrderBook& orderBook,
                                                 Wallet& wallet,
                                                 int year,
                                                 int ordersPerSide = 5);

    private:
        struct Row
        {
            std::string username, timestamp, product, orderType;
            double price, amount;
        };
        std::vector<Row> readAllForUser(const std::string& username);
        static std::string tradeLabel(OrderBookType type);
        static double driftForProduct(const std::string& product);

        const std::string tradingFile = "USERS_TRADING.CSV";
};
