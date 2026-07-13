// START MY CODE
#pragma once
#include <string>
#include <vector>
#include "OrderBookEntry.h"

/** Task 3: owns USERS_TRADING.CSV — transaction history, reporting, and stats.
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

    private:
        struct Row
        {
            std::string username, timestamp, product, orderType;
            double price, amount;
            std::string status;
        };
        std::vector<Row> readAllForUser(const std::string& username);
        static std::string tradeLabel(OrderBookType type);

        const std::string tradingFile = "USERS_TRADING.CSV";
};
// END MY CODE
