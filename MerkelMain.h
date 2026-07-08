#pragma once

#include <string>
#include "OrderBook.h"
#include "Wallet.h"
#include "TransactionManager.h"
#include "Simulation.h"

/** One logged-in trading session: constructed per login, destroyed on logout */
class MerkelMain
{
    public:
        MerkelMain(std::string _username, OrderBook& _orderBook);
        /** logged-in menu loop; returns on logout */
        void run();

    private:
        // submenus grouped by user intention; each loops until 0 = Back
        void marketMenu();
        void walletMenu();
        void simulationMenu();

        // features
        void printOHLC();
        void enterAsk();
        void enterBid();
        void printWallet();
        void depositFunds();
        void withdrawFunds();
        void gotoNextTimeframe();
        /** persist the wallet to USERS_WALLET.CSV*/
        void persistWallet();
        int currentSystemYear();

        std::string username;
        std::string currentTime;
        OrderBook& orderBook;   // shared market data, loaded once in main()
        Wallet wallet;
        TransactionManager transactionManager;
};
