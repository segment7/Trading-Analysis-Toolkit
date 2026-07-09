#include "MerkelMain.h"
#include <iostream>
#include <vector>
#include "Candlestick.h"
#include "Input.h"
#include <iomanip>
#include <ctime>

MerkelMain::MerkelMain(std::string _username, OrderBook& _orderBook)
// Constructor initialisation
: username(_username), orderBook(_orderBook)
{
    wallet.loadFromFile(username);
    if (wallet.initialiseSignupBonusIfEmpty(username))
    {
        std::cout << "First login: signup bonus of 200 BTC, 200 ETH, 200 USDT added to your wallet." << std::endl;
    }
}

void MerkelMain::run()
{
    while (true)
    {
        std::cout << "\n--------------------------------" << std::endl;
        std::cout << "--- Logged in as " << username << " ---" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        std::cout << "\n1: Market" << std::endl;
        std::cout << "2: Wallet" << std::endl;
        std::cout << "0: Logout\n" << std::endl;
        int choice = Input::promptChoice(0, 2);
        if (choice == 0)
        {
            std::cout << "Logged out." << std::endl;
            return;
        }
        if (choice == 1) marketMenu();
        if (choice == 2) walletMenu();
    }
}

void MerkelMain::marketMenu()
{
    while (true)
    {
        std::cout << "\n--- Market ---" << std::endl;
        std::cout << "\nAvailable products:" << std::endl;
        for (const std::string& product : orderBook.getKnownProducts())
        {
            std::cout << product << std::endl;
        }

        std::cout << "\n1: Print OHLC summary" << std::endl;
        std::cout << "0: Back\n" << std::endl;
        int choice = Input::promptChoice(0, 1);
        if (choice == 0) return;
        if (choice == 1) printOHLC();
    }
}

void MerkelMain::walletMenu()
{
    while (true)
    {
        // balance is shown automatically on entry and after every action,
        // so no separate "show balance" option is needed
        std::cout << "\n--- Wallet ---\n" << std::endl;
        printWallet();
        std::cout << "1: Deposit" << std::endl;
        std::cout << "2: Withdraw" << std::endl;
        std::cout << "3: Recent transactions" << std::endl;
        std::cout << "4: Activity statistics" << std::endl;
        std::cout << "5: Simulation" << std::endl;
        std::cout << "0: Back\n" << std::endl;
        int choice = Input::promptChoice(0, 5);
        if (choice == 0) return;
        if (choice == 1) depositFunds();
        if (choice == 2) withdrawFunds();
        if (choice == 3) transaction.printRecent(username, 5);
        if (choice == 5) simulationMenu();
        if (choice == 4)
        {
            std::cout << "Start date YYYY/MM/DD (blank for all): ";
            std::string startDate;
            std::getline(std::cin, startDate);
            std::cout << "End date YYYY/MM/DD (blank for all): ";
            std::string endDate;
            std::getline(std::cin, endDate);
            if ((!startDate.empty() && startDate.size() != 10) ||
                (!endDate.empty()   && endDate.size()   != 10))
            {
                std::cout << "Dates must be YYYY/MM/DD." << std::endl;
            }
            else if (!startDate.empty() && !endDate.empty() && startDate > endDate)
            {
                std::cout << "Start date must not be after end date." << std::endl;
            }
            else
            {
                transaction.printActivityStats(username, startDate, endDate);
            }
        }
    }
}

void MerkelMain::simulationMenu()
{
    Simulation simulation{username, orderBook, wallet, transaction, currentSystemYear()};

    while (true)
    {
        std::cout << "\n--- Simulation ---" << std::endl;
        std::cout << "\n1: Start" << std::endl;
        std::cout << "0: Back\n" << std::endl;
        int choice = Input::promptChoice(0, 1);
        if (choice == 0) return;
        if (choice == 1)
        {
            // wallet is persisted per fill inside generateTransactions
            std::pair<int, int> result = simulation.start();
            std::cout << "Simulated " << (result.first + result.second) << " orders: "
                      << result.first << " filled, "
                      << result.second << " skipped (insufficient balance)." << std::endl;
        }
    }
}

/** Task 1: OHLC summary table — yearly by default, daily when a date range is given */
void MerkelMain::printOHLC()
{
    std::cout << "Enter product (e.g. ETH/USDT): ";
    std::string product;
    std::getline(std::cin, product);
    if (!orderBook.hasKnownProduct(product))
    {
        std::cout << "Unknown product: " << product << std::endl;
        return;
    }
     std::cout << "\n--- OHLC Summary ---" << std::endl;

    // show asks and bids together via one reusable lambda
    auto printTable = [&](OrderBookType type, const std::string& label,
                          const std::string& startDate, const std::string& endDate,
                          int groupByChars)
    {
        std::vector<Candlestick> candles = orderBook.computeOHLC(type, product,
                                                                 startDate, endDate,
                                                                 groupByChars);
        std::cout << "\n" << label << " on " << product << ":" << std::endl;
        if (candles.empty())
        {
            std::cout << "No data in this range." << std::endl;
            return;
        }
        std::cout << std::fixed << std::setprecision(8);
        std::cout << std::left  << std::setw(12) << "Date"
                  << std::right << std::setw(16) << "Open"
                  << std::setw(16) << "High"
                  << std::setw(16) << "Low"
                  << std::setw(16) << "Close" << std::endl;
        for (Candlestick& c : candles)
        {
            std::cout << std::left  << std::setw(12) << c.date
                      << std::right << std::setw(16) << c.open
                      << std::setw(16) << c.high
                      << std::setw(16) << c.low
                      << std::setw(16) << c.close << std::endl;
        }
    };

    // annual summary shows by default
    printTable(OrderBookType::ask, "Asks", "", "", 4);
    printTable(OrderBookType::bid, "Bids", "", "", 4);

    // opt-in follow-ups: date filter, or gap-fill init so missing years appear here
    while (true)
    {
        std::cout << "\n1: Filter" << std::endl;
        std::cout << "2: Generate" << std::endl;
        std::cout << "0: Back\n" << std::endl;
        int followUp = Input::promptChoice(0, 2);
        if (followUp == 0) return;

        if (followUp == 2)
        {
            // gap-fill is market data maintenance, so it lives inside the OHLC view it feeds
            Simulation simulation{username, orderBook, wallet, transaction, currentSystemYear()};
            simulation.init();
            // refreshed yearly summary shows the newly filled years
            printTable(OrderBookType::ask, "Asks", "", "", 4);
            printTable(OrderBookType::bid, "Bids", "", "", 4);
            continue;
        }

        // followUp == 1: date-filtered daily tables
        std::string startDate, endDate;
        std::cout << "Start date YYYY/MM/DD (blank for no lower bound): ";
        std::getline(std::cin, startDate);
        std::cout << "End date YYYY/MM/DD (blank for no upper bound): ";
        std::getline(std::cin, endDate);
        if ((!startDate.empty() && startDate.size() != 10) ||
            (!endDate.empty()   && endDate.size()   != 10))
        {
            std::cout << "Dates must be YYYY/MM/DD." << std::endl;
            continue;
        }
        if (!startDate.empty() && !endDate.empty() && startDate > endDate)
        {
            std::cout << "Start date must not be after end date." << std::endl;
            continue;
        }

        printTable(OrderBookType::ask, "Asks", startDate, endDate, 10);
        printTable(OrderBookType::bid, "Bids", startDate, endDate, 10);
        return;
    }
}

void MerkelMain::printWallet()
{
    std::cout << wallet.toString() << std::endl;
}

void MerkelMain::depositFunds()
{
    std::cout << "Currency to deposit (e.g. USDT): ";
    std::string currency;
    std::getline(std::cin, currency);
    if (currency.empty())
    {
        std::cout << "Currency is required." << std::endl;
        return;
    }
    std::cout << "Amount: ";
    std::string amountStr;
    std::getline(std::cin, amountStr);
    try
    {
        double amount = std::stod(amountStr);
        if (amount <= 0)
        {
            std::cout << "Amount must be positive." << std::endl;
            return;
        }
        wallet.insertCurrency(currency, amount);
        persistWallet();
        std::cout << "Deposited " << amount << " " << currency << "." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Invalid amount." << std::endl;
    }
}

void MerkelMain::withdrawFunds()
{
    std::cout << "Currency to withdraw (e.g. USDT): ";
    std::string currency;
    std::getline(std::cin, currency);
    if (currency.empty())
    {
        std::cout << "Currency is required." << std::endl;
        return;
    }
    std::cout << "Amount: ";
    std::string amountStr;
    std::getline(std::cin, amountStr);
    try
    {
        double amount = std::stod(amountStr);
        if (amount <= 0)
        {
            std::cout << "Amount must be positive." << std::endl;
            return;
        }
        if (wallet.removeCurrency(currency, amount))
        {
            persistWallet();
            std::cout << "Withdrew " << amount << " " << currency << "." << std::endl;
        }
        else
        {
            std::cout << "Insufficient funds." << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "Invalid amount." << std::endl;
    }
}

void MerkelMain::persistWallet()
{
    wallet.saveToFile(username);
}

int MerkelMain::currentSystemYear()
{
    std::time_t now = std::time(nullptr);
    return std::localtime(&now)->tm_year + 1900;
}
