#include "TransactionManager.h"
#include "CSVReader.h"
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>
#include <random>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace
{
    // Task 4 price model: seed * (1+drift)^(year-2020) * (1+noise), ±1% ask/bid spread
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
    // insertOrder re-sorts by timestamp, so generation order doesn't matter.
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

TransactionManager::TransactionManager()
{
}

double TransactionManager::driftForProduct(const std::string& product)
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

std::string TransactionManager::tradeLabel(OrderBookType type)
{
    // asksale/bidsale -> plain "ask"/"bid" for the CSV and stats;
    // skipped orders arrive as plain ask/bid (they were never settled)
    if (type == OrderBookType::asksale || type == OrderBookType::ask) return "ask";
    if (type == OrderBookType::bidsale || type == OrderBookType::bid) return "bid";
    return "unknown";
}

void TransactionManager::logTransaction(const std::string& username, const OrderBookEntry& sale,
                                        const std::string& status)
{
    std::ofstream file{tradingFile, std::ios::app};
    file << username << "," << sale.timestamp << "," << sale.product << ","
         << tradeLabel(sale.orderType) << "," << sale.price << "," << sale.amount << ","
         << status << std::endl;
}

std::vector<TransactionManager::Row> TransactionManager::readAllForUser(const std::string& username)
{
    std::vector<Row> rows;
    std::ifstream file{tradingFile};
    std::string line;
    while (std::getline(file, line))
    {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if ((tokens.size() != 6 && tokens.size() != 7) || tokens[0] != username) continue;
        try
        {
            rows.push_back(Row{tokens[0], tokens[1], tokens[2], tokens[3],
                               std::stod(tokens[4]), std::stod(tokens[5]),
                               tokens.size() == 7 ? tokens[6] : "filled"});
        }
        catch (const std::exception& e) { }   // skip malformed rows
    }
    return rows;   // missing file -> empty; append-only CSV keeps rows chronological
}

void TransactionManager::printRecent(const std::string& username, int n)
{
    std::vector<Row> rows = readAllForUser(username);
    if (rows.empty())
    {
        std::cout << "\nNo transactions yet.\n" << std::endl;
        return;
    }
    int start = std::max(0, static_cast<int>(rows.size()) - n);
    std::cout << "Last " << (rows.size() - start) << " transaction(s):" << std::endl;
    for (int i = static_cast<int>(rows.size()) - 1; i >= start; --i)
    {
        Row& r = rows[i];
        std::cout << r.timestamp << "  " << r.product << "  " << r.orderType
                  << "  price=" << r.price << "  amount=" << r.amount
                  << "  " << r.status << std::endl;
    }
}

void TransactionManager::printActivityStats(const std::string& username,
                                            const std::string& startDate,
                                            const std::string& endDate)
{
    std::vector<Row> rows = readAllForUser(username);
    if (rows.empty())
    {
        std::cout << "\nNo transactions yet.\n" << std::endl;
        return;
    }

    int totalAsks = 0, totalBids = 0, totalSkipped = 0;
    double totalSpent = 0;
    std::map<std::string, std::pair<int, int>> perProduct;   // product -> (asks, bids)

    for (Row& r : rows)
    {
        if (r.status == "skipped") { totalSkipped++; continue; }   // never settled — excluded from totals
        if (r.orderType == "ask") { totalAsks++; perProduct[r.product].first++;  }
        if (r.orderType == "bid") { totalBids++; perProduct[r.product].second++; }

        // same date-range convention as OrderBook::getOrdersInRange
        std::string datePart = r.timestamp.substr(0, 10);
        if (datePart >= startDate && (endDate.empty() || datePart <= endDate))
        {
            totalSpent += r.price * r.amount;
        }
    }

    std::cout << "Total asks: " << totalAsks << ", total bids: " << totalBids << std::endl;
    for (auto const& kv : perProduct)
    {
        std::cout << "  " << kv.first << ": " << kv.second.first << " asks, "
                  << kv.second.second << " bids" << std::endl;
    }
    std::cout << "Total spent in range: " << totalSpent << std::endl;
    std::cout << "Skipped (insufficient balance): " << totalSkipped << std::endl;
}

std::pair<int, int> TransactionManager::generateTransactions(const std::string& username,
                                                              OrderBook& orderBook,
                                                              Wallet& wallet,
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

            // resting order defaults to "dataset" — visible to OHLC, never re-settled
            OrderBookEntry restingAsk{askPrice, amount, timestamp, product, OrderBookType::ask};
            // this user's actual fill, settled against their wallet now
            OrderBookEntry fillAsk{sellPrice, amount, timestamp, product, OrderBookType::asksale, username};

            if (wallet.canFulfillOrder(fillAsk))
            {
                orderBook.insertOrder(restingAsk);
                wallet.processSale(fillAsk);
                if (persist)
                {
                    logTransaction(username, fillAsk);
                    wallet.saveToFile(username);   // balance persisted after every transaction
                }
                filled++;
            }
            else
            {
                // insufficient balance — Task 4.2.iii; recorded so the history shows it
                if (persist) logTransaction(username, fillAsk, "skipped");
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
            OrderBookEntry fillBid{buyPrice, amount, timestamp, product, OrderBookType::bidsale, username};

            if (wallet.canFulfillOrder(fillBid))
            {
                orderBook.insertOrder(restingBid);
                wallet.processSale(fillBid);
                if (persist)
                {
                    logTransaction(username, fillBid);
                    wallet.saveToFile(username);
                }
                filled++;
            }
            else
            {
                if (persist) logTransaction(username, fillBid, "skipped");
                skipped++;
            }
        }
    }

    return {filled, skipped};
}
