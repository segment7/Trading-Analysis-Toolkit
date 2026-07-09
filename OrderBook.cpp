#include "OrderBook.h"
#include "CSVReader.h"
#include <map>
#include <algorithm>

/** construct, reading a csv data file */
OrderBook::OrderBook(std::string filename)
{
    orders = CSVReader::readCSV(filename);
}

/** return vector of all known products in the dataset */
std::vector<std::string> OrderBook::getKnownProducts()
{
    std::vector<std::string> products;
    std::map<std::string, bool> prodMap;

    for (OrderBookEntry& e : orders)
    {
        prodMap[e.product] = true;
    }

    for (auto const& e : prodMap)
    {
        products.push_back(e.first);
    }

    return products;
}

/** Filter logic core function - one loop, callers supply the condition as a lambda */
std::vector<OrderBookEntry> OrderBook::filterOrders(const std::function<bool(const OrderBookEntry&)>& pred)
{
    std::vector<OrderBookEntry> orders_sub;
    std::copy_if(orders.begin(), orders.end(), std::back_inserter(orders_sub), pred);
    return orders_sub;
}

std::vector<OrderBookEntry> OrderBook::getOrdersInRange(OrderBookType type,
                                                        const std::string& product,
                                                        const std::string& startDate,
                                                        const std::string& endDate)
{
    return filterOrders([&](const OrderBookEntry& e)
    {
        if (e.orderType != type || e.product != product) return false;
        // compare date prefix only, so endDate stays inclusive; empty bounds need no special case
        std::string datePart = e.timestamp.substr(0, 10);
        return datePart >= startDate && (endDate.empty() || datePart <= endDate);
    });
}

std::vector<Candlestick> OrderBook::computeOHLC(OrderBookType type,
                                                const std::string& product,
                                                const std::string& startDate,
                                                const std::string& endDate,
                                                int groupByChars)
{
    std::vector<Candlestick> candles;
    // chronological order, so first/last seen = open/close;
    std::vector<OrderBookEntry> filtered = getOrdersInRange(type, product, startDate, endDate);
    for (OrderBookEntry& e : filtered)
    {
        std::string key = e.timestamp.substr(0, groupByChars);
        if (candles.empty() || candles.back().date != key)
        {
            // first entry of a new period: open = high = low = close = this price
            candles.push_back(Candlestick{key, e.price, e.price, e.price, e.price});
        }
        else
        {
            Candlestick& c = candles.back();
            if (e.price > c.high) c.high = e.price;
            if (e.price < c.low)  c.low  = e.price;
            c.close = e.price;   // last one wins = close
        }
    }
    return candles;
}

double OrderBook::getSeedPrice(OrderBookType type, const std::string& product)
{
    std::vector<OrderBookEntry> matches = filterOrders([&](const OrderBookEntry& e)
    {
        return e.orderType == type && e.product == product && e.username == "dataset";
    });
    if (matches.empty()) return 100.0;   // fallback if a product has no real data of this type
    return matches.back().price;         // orders stays chronological, so back() = most recent
}

void OrderBook::insertOrder(OrderBookEntry& order)
{
    orders.push_back(order);
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
}
