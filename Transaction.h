#pragma once
#include <string>
#include <vector>
#include <utility>
#include "OrderBookEntry.h"
#include "OrderBook.h"
#include "Wallet.h"

/** Task 3: owns USERS_TRADING.CSV — logs completed trades, shows history & stats.
 *  Row format: username,timestamp,product,orderType,price,amount,status
 *  (orderType "ask"/"bid"; status "filled" or "skipped" for insufficient balance) */
class Transaction
{
    public:
        Transaction();

        /** append one trade for a user: "filled" right after the wallet is updated,
         *  or "skipped" when the wallet could not cover the order */
        void logTransaction(const std::string& username, const OrderBookEntry& sale,
                            const std::string& status = "filled");

        /** print the last n transactions for this user (any product), most recent first */
        void printRecent(const std::string& username, int n = 5);

        /** print ask/bid counts (all products + per product) and total spent in [startDate,endDate] */
        void printActivityStats(const std::string& username,
                                const std::string& startDate,
                                const std::string& endDate);

        /** Task 4: generate ordersPerSide asks + bids per product for a year, priced from
         *  a seed+drift+noise model, settled against the wallet and logged.
         *  The user's fills cross the spread (sell at mid*0.99, buy at mid*1.01) so
         *  simulated trading costs money rather than minting it. liveTimestamps=true
         *  stamps orders with the real current time instead of random moments in the year.
         *  persist=false is for the system gap-fill: its trades are market data only,
         *  which will not be written to the user trading log or wallet file.
         *  Returns {filled, skipped}. */
        std::pair<int, int> generateTransactions(const std::string& username,
                                                 OrderBook& orderBook,
                                                 Wallet& wallet,
                                                 int year,
                                                 int ordersPerSide = 5,
                                                 bool liveTimestamps = false,
                                                 bool persist = true);

    private:
        struct Row
        {
            std::string username, timestamp, product, orderType;
            double price, amount;
            std::string status;
        };
        std::vector<Row> readAllForUser(const std::string& username);
        static std::string tradeLabel(OrderBookType type);
        static double driftForProduct(const std::string& product);

        const std::string tradingFile = "USERS_TRADING.CSV";
};
