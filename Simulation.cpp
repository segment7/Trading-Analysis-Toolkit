#include "Simulation.h"
#include "CSVReader.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace
{
    // a few orders per product/side/year — enough to seed one OHLC candle
    constexpr int INIT_ORDERS_PER_SIDE = 2;
}

Simulation::Simulation(std::string _username, OrderBook& _orderBook, Wallet& _wallet,
                       TransactionManager& _transactionManager, int _currentYear)
: username(_username), orderBook(_orderBook), wallet(_wallet),
  transactionManager(_transactionManager), currentYear(_currentYear)
{
    // inert on purpose — gap-fill only runs via explicit init()
}

bool Simulation::yearHasData(int year)
{
    std::vector<std::string> products = orderBook.getKnownProducts();
    if (products.empty()) return true;   // nothing to check against

    std::string yearStart = std::to_string(year) + "/01/01";
    std::string yearEnd   = std::to_string(year) + "/12/31";
    return !orderBook.getOrdersInRange(OrderBookType::ask, products.front(),
                                       yearStart, yearEnd).empty();
}

bool Simulation::isReady()
{
    for (int year = OrderBook::DATASET_YEAR + 1; year < currentYear; year++)
    {
        if (!yearHasData(year)) return false;
    }
    return true;
}

int Simulation::init()
{
    // backfill each year from the dataset year up to (not including) the current year
    std::vector<std::string> products = orderBook.getKnownProducts();
    if (products.empty()) return 0;

    // derive currencies once, outside the year loop
    std::vector<std::string> currencies;
    for (const std::string& product : products)
    {
        for (const std::string& currency : CSVReader::tokenise(product, '/'))
        {
            if (std::find(currencies.begin(), currencies.end(), currency) == currencies.end())
            {
                currencies.push_back(currency);
            }
        }
    }

    int yearsFilled = 0;
    for (int year = OrderBook::DATASET_YEAR + 1; year < currentYear; year++)
    {
        if (yearHasData(year))
        {
            std::cout << "Year " << year << ": already initialised." << std::endl;
            continue;
        }

        std::cout << "Year " << year << ": generating market data..." << std::endl;

        // throwaway, generously-funded wallet — never persisted
        Wallet systemWallet;
        for (const std::string& currency : currencies)
        {
            systemWallet.insertCurrency(currency, 1000000000.0);
        }
        std::pair<int, int> result = transactionManager.generateTransactions(
            "simuser", orderBook, systemWallet, year, INIT_ORDERS_PER_SIDE);

        std::cout << "Year " << year << ": done (" << result.first << " filled, "
                  << result.second << " skipped)." << std::endl;
        yearsFilled++;
    }

    if (yearsFilled == 0)
    {
        std::cout << "Market history already up to date." << std::endl;
    }
    return yearsFilled;
}

std::pair<int, int> Simulation::start()
{
    // scale ordersPerSide up if there are fewer than 5 products
    int productCount = static_cast<int>(orderBook.getKnownProducts().size());
    int ordersPerSide = productCount > 0
        ? static_cast<int>(std::ceil(50.0 / (productCount * 2)))
        : 5;

    // fulfill any more orders until reach 50
    int totalFilled = 0, totalSkipped = 0;
    for (int batch = 0; batch < 10 && totalFilled < 50; batch++)
    {
        std::pair<int, int> result = transactionManager.generateTransactions(
            username, orderBook, wallet, currentYear, ordersPerSide);
        totalFilled += result.first;
        totalSkipped += result.second;
    }
    return {totalFilled, totalSkipped};
}
