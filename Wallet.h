#pragma once

#include <string>
#include <map>
#include "OrderBookEntry.h"
#include <iostream>

class Wallet 
{
    public:
        Wallet();
        /** insert currency to the wallet */
        void insertCurrency(std::string type, double amount);
        /** remove currency from the wallet */
        bool removeCurrency(std::string type, double amount);
        
        /** check if the wallet contains this much currency or more */
        bool containsCurrency(std::string type, double amount);
        /** checks if the wallet can cope with this ask or bid.*/
        bool canFulfillOrder(OrderBookEntry order);
        /** update the contents of the wallet
         * assumes the order was made by the owner of the wallet
        */
        void processSale(OrderBookEntry& sale);

// START MY CODE
        /** Task 3: load this user's balances from USERS_WALLET.CSV (missing file/user -> empty wallet) */
        void loadFromFile(const std::string& username);
        /** Task 3: persist current balances back to USERS_WALLET.CSV, preserving other users' rows */
        void saveToFile(const std::string& username) const;
        /** Task 2.1.6: give a new wallet its first-login bonus and persist it */
        bool initialiseSignupBonusIfEmpty(const std::string& username);
// END MY CODE

        /** generate a string representation of the wallet */
        std::string toString();
        friend std::ostream& operator<<(std::ostream& os, Wallet& wallet);


    private:
        std::map<std::string,double> currencies;
// START MY CODE
        const std::string walletFile = "USERS_WALLET.CSV";
// END MY CODE

};



	
