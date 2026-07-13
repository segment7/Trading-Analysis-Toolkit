// START MY CODE
#include "Simulation.h"
#include "CSVReader.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <map>
#include <random>
#include <sstream>

namespace
{
    // a few orders per product/side/year — enough to seed one OHLC candle
    constexpr int INIT_ORDERS_PER_SIDE = 2;

    // Task 4 price model: seed * (1+drift)^(year-2020) * (1+noise), +/-1% ask/bid spread
    double randomNoise()
    {
        static std::mt19937 rng{std::random_device{}()};
        static std::uniform_real_distribution<double> dist{-0.05, 0.05};
        return dist(rng);
    }

    double randomAmount()
    {
        static std::mt19937 rng{std::random_device{}()};
        static std::uniform_real_distribution<double> dist{0.1, 3.0};
        return dist(rng);
    }

    // random moment spread across the year; current year is capped at "now".
    // OrderBook::insertOrder re-sorts by timestamp, so generation order doesn't matter.
    std::string makeTimestamp(int year, int offsetSeconds)
    {
        std::time_t now = std::time(nullptr);
        int currentYear = std::localtime(&now)->tm_year + 1900;

        std::tm startTm = {};
        startTm.tm_year = year - 1900;
        startTm.tm_mon = 0;
        startTm.tm_mday = 1;
        std::time_t yearStart = std::mktime(&startTm);

        std::time_t yearEnd;
        if (year == currentYear)
        {
            yearEnd = now;
        }
        else
        {
            std::tm endTm = {};
            endTm.tm_year = year - 1900;
            endTm.tm_mon = 11;
            endTm.tm_mday = 31;
            endTm.tm_hour = 23;
            endTm.tm_min = 59;
            endTm.tm_sec = 59;
            yearEnd = std::mktime(&endTm);
        }
        if (yearEnd <= yearStart) yearEnd = yearStart + 1;   // guard a same-second edge case

        static std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<long long> dist{static_cast<long long>(yearStart),
                                                       static_cast<long long>(yearEnd)};
        std::time_t chosen = static_cast<std::time_t>(dist(rng)) + offsetSeconds;
        if (chosen > yearEnd) chosen = yearEnd;   // keep the current-year cap intact

        std::tm chosenTm = *std::localtime(&chosen);
        std::ostringstream oss;
        oss << std::put_time(&chosenTm, "%Y/%m/%d %H:%M:%S") << ".000000";
        return oss.str();
    }

    // the real wall-clock moment, for live simulation runs
    std::string nowTimestamp()
    {
        std::time_t now = std::time(nullptr);
        std::tm nowTm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&nowTm, "%Y/%m/%d %H:%M:%S") << ".000000";
        return oss.str();
    }
}

Simulation::Simulation(std::string _username, OrderBook& _orderBook, Wallet& _wallet,
                       Transaction& _transaction, int _currentYear)
: username(_username), orderBook(_orderBook), wallet(_wallet),
  transaction(_transaction), currentYear(_currentYear)
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

double Simulation::driftForProduct(const std::string& product)
{
    // fixed per-product drift gives each product's simulated years some uniqueness
    static const std::map<std::string, double> drift = {
        {"BTC/USDT",  0.08},
        {"ETH/USDT",  0.10},
        {"DOGE/USDT", 0.05},
        {"ETH/BTC",   0.02},
        {"DOGE/BTC", -0.03},
    };
    auto it = drift.find(product);
    return it != drift.end() ? it->second : 0.05;   // fallback for an unlisted product
}

std::pair<int, int> Simulation::generateTransactions(const std::string& actor,
                                                     Wallet& targetWallet,
                                                     int year,
                                                     int ordersPerSide,
                                                     bool liveTimestamps,
                                                     bool persist)
{
    int filled = 0, skipped = 0;
    int offsetSeconds = 0;   // spaces out timestamps so insertOrder's sort keeps them in order

    for (const std::string& product : orderBook.getKnownProducts())
    {
        double seed = orderBook.getSeedPrice(OrderBookType::ask, product);
        double yearFactor = std::pow(1.0 + driftForProduct(product), year - OrderBook::DATASET_YEAR);

        for (int i = 0; i < ordersPerSide; i++)   // asks
        {
            std::string timestamp = liveTimestamps ? nowTimestamp()
                                                   : makeTimestamp(year, offsetSeconds++);
            double mid = seed * yearFactor * (1.0 + randomNoise());
            double askPrice = mid * 1.01;    // resting book ask (market data for OHLC)
            double sellPrice = mid * 0.99;   // the user's sell crosses the spread: it hits the best bid
            double amount = randomAmount();

            // resting order defaults to "dataset" - visible to OHLC, never re-settled
            OrderBookEntry restingAsk{askPrice, amount, timestamp, product, OrderBookType::ask};
            // this actor's actual fill, settled against their wallet now
            OrderBookEntry fillAsk{sellPrice, amount, timestamp, product, OrderBookType::asksale, actor};

            if (targetWallet.canFulfillOrder(fillAsk))
            {
                orderBook.insertOrder(restingAsk);
                targetWallet.processSale(fillAsk);
                if (persist)
                {
                    transaction.logTransaction(actor, fillAsk);
                    targetWallet.saveToFile(actor);   // balance persisted after every transaction
                }
                filled++;
            }
            else
            {
                // insufficient balance - Task 4.2.iii; recorded so the history shows it
                if (persist) transaction.logTransaction(actor, fillAsk, "skipped");
                skipped++;
            }
        }

        for (int i = 0; i < ordersPerSide; i++)   // bids
        {
            std::string timestamp = liveTimestamps ? nowTimestamp()
                                                   : makeTimestamp(year, offsetSeconds++);
            double mid = seed * yearFactor * (1.0 + randomNoise());
            double bidPrice = mid * 0.99;   // resting book bid (market data for OHLC)
            double buyPrice = mid * 1.01;   // the user's buy crosses the spread: it lifts the best ask
            double amount = randomAmount();

            OrderBookEntry restingBid{bidPrice, amount, timestamp, product, OrderBookType::bid};
            OrderBookEntry fillBid{buyPrice, amount, timestamp, product, OrderBookType::bidsale, actor};

            if (targetWallet.canFulfillOrder(fillBid))
            {
                orderBook.insertOrder(restingBid);
                targetWallet.processSale(fillBid);
                if (persist)
                {
                    transaction.logTransaction(actor, fillBid);
                    targetWallet.saveToFile(actor);
                }
                filled++;
            }
            else
            {
                if (persist) transaction.logTransaction(actor, fillBid, "skipped");
                skipped++;
            }
        }
    }

    return {filled, skipped};
}

int Simulation::init()
{
    // backfill each year from the dataset year through the current year
    // (the current year's timestamps are capped at "now" by the generator)
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
    for (int year = OrderBook::DATASET_YEAR + 1; year <= currentYear; year++)
    {
        if (yearHasData(year))
        {
            std::cout << "Year " << year << ": already initialised." << std::endl;
            continue;
        }

        std::cout << "Year " << year << ": generating market data..." << std::endl;

        // throwaway, generously-funded system wallet — its trades are market data,
        // (persist=false)
        Wallet systemWallet;
        for (const std::string& currency : currencies)
        {
            systemWallet.insertCurrency(currency, 1000000000.0);
        }
        std::pair<int, int> result = generateTransactions(
            "system", systemWallet, year, INIT_ORDERS_PER_SIDE,
            false /* liveTimestamps */, false /* persist */);

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
        std::pair<int, int> result = generateTransactions(
            username, wallet, currentYear, ordersPerSide,
            true /* live : user? : system */,
            true /* persist : user? : system */);
        totalFilled += result.first;
        totalSkipped += result.second;
    }
    return {totalFilled, totalSkipped};
}
// END MY CODE
