#pragma once
#include <string>
#include <utility>
#include "OrderBook.h"
#include "Wallet.h"
#include "TransactionManager.h"

/** Task 4: fills market-history gaps (init) and runs the user's simulated trades (start).
 *  Construction is inert; init() must run before start(). Idempotent per year. */
class Simulation
{
    public:
        Simulation(std::string username, OrderBook& orderBook, Wallet& wallet,
                  TransactionManager& transactionManager, int currentYear);

        /** explicit gap-fill, one year at a time with progress printed per year;
         *  returns how many years were newly filled */
        int init();

        /** true once every year between the dataset and today already has market
         *  data (i.e. init() has completed, this session or a previous one) */
        bool isReady();

        /** run 50 transactions for the real user at the current system year —
         *  caller should check isReady() first */
        std::pair<int, int> start();

    private:
        /** shared by init() and isReady(): does this year already have data? */
        bool yearHasData(int year);

        std::string username;
        OrderBook& orderBook;
        Wallet& wallet;
        TransactionManager& transactionManager;
        int currentYear;
};
