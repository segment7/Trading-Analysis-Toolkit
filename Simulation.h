#pragma once
#include <string>
#include <utility>
#include "OrderBook.h"
#include "Wallet.h"
#include "Transaction.h"

/** Task 4: fills market-history gaps (init) and runs the user's simulated trades (start).
 *  Construction is inert; both operations are independent and init() is idempotent per year. */
class Simulation
{
    public:
        Simulation(std::string username, OrderBook& orderBook, Wallet& wallet,
                  Transaction& transaction, int currentYear);

        /** explicit gap-fill from the dataset year through the current year,
         *  one year at a time with progress printed per year;
         *  returns how many years were newly filled */
        int init();

        /** run at least 50 transactions stamped with the real current time */
        std::pair<int, int> start();

    private:
        /** does this year already have data? (keeps init() idempotent) */
        bool yearHasData(int year);

        /** Task 4: simulation-owned generator for synthetic asks/bids.
         *  It updates the wallet through Wallet, market data through OrderBook,
         *  and history through Transaction, returning {filled, skipped}. */
        std::pair<int, int> generateTransactions(const std::string& actor,
                                                 Wallet& targetWallet,
                                                 int year,
                                                 int ordersPerSide,
                                                 bool liveTimestamps,
                                                 bool persist);

        /** fixed product drift for the Task 4 synthetic price model */
        static double driftForProduct(const std::string& product);

        std::string username;
        OrderBook& orderBook;
        Wallet& wallet;
        Transaction& transaction;
        int currentYear;
};
