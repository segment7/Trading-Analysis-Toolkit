// START MY CODE
#include "Transaction.h"
#include "CSVReader.h"
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>

Transaction::Transaction()
{
}

std::string Transaction::tradeLabel(OrderBookType type)
{
    // asksale/bidsale -> plain "ask"/"bid" for the CSV and stats;
    // skipped orders arrive as plain ask/bid (they were never settled)
    if (type == OrderBookType::asksale || type == OrderBookType::ask) return "ask";
    if (type == OrderBookType::bidsale || type == OrderBookType::bid) return "bid";
    return "unknown";
}

void Transaction::logTransaction(const std::string& username, const OrderBookEntry& sale,
                                        const std::string& status)
{
    std::ofstream file{tradingFile, std::ios::app};
    file << username << "," << sale.timestamp << "," << sale.product << ","
         << tradeLabel(sale.orderType) << "," << sale.price << "," << sale.amount << ","
         << status << std::endl;
}

std::vector<Transaction::Row> Transaction::readAllForUser(const std::string& username)
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

void Transaction::printRecent(const std::string& username, int n)
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

void Transaction::printActivityStats(const std::string& username,
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
// END MY CODE
