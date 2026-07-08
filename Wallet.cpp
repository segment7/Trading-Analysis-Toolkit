#include "Wallet.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "CSVReader.h"

Wallet::Wallet()
{


}

void Wallet::insertCurrency(std::string type, double amount)
{
    double balance;
    if (amount < 0)
    {
        throw std::exception{};
    }
    if (currencies.count(type) == 0) // not there yet
    {
        balance = 0;
    }
    else { // is there 
        balance = currencies[type];
    }
    balance += amount; 
    currencies[type] = balance; 
}

bool Wallet::removeCurrency(std::string type, double amount)
{
    if (amount < 0)
    {
        return false; 
    }
    if (currencies.count(type) == 0) // not there yet
    {
        //std::cout << "No currency for " << type << std::endl;
        return false;
    }
    else { // is there - do  we have enough
        if (containsCurrency(type, amount))// we have enough
        {
            //std::cout << "Removing " << type << ": " << amount << std::endl;
            currencies[type] -= amount;
            return true;
        } 
        else // they have it but not enough.
            return false; 
    }
}

bool Wallet::containsCurrency(std::string type, double amount)
{
    if (currencies.count(type) == 0) // not there yet
        return false;
    else 
        return currencies[type] >= amount;
    
}

std::string Wallet::toString()
{
    std::string s;
    for (std::pair<std::string,double> pair : currencies)
    {
        std::string currency = pair.first;
        double amount = pair.second;
        s += currency + " : " + std::to_string(amount) + "\n";
    }
    return s;
}

bool Wallet::canFulfillOrder(OrderBookEntry order)
{
    std::vector<std::string> currs = CSVReader::tokenise(order.product, '/');
    // defence in depth: a product that isn't BASE/QUOTE would make currs[1]
    // an out-of-bounds read below — treat it as unaffordable instead
    if (currs.size() != 2) return false;
    bool ok = false;
    double amount = order.amount;        
    std::string currency = currs[0];
    bool isAsk = order.orderType == OrderBookType::ask || order.orderType == OrderBookType::asksale;
    // ask/asksale: selling the base currency
    if (isAsk)
    {
        ok = containsCurrency(currs[0], order.amount);
    }
    // bid/bidsale: paying with the quote currency
    else
    {
        ok = containsCurrency(currs[1], order.amount * order.price);
    }

    std::string typeLabel = isAsk ? "ask" : "bid";
    std::cout << "canFulfillOrder [" << typeLabel << "] "
              << currency 
              << " : " 
              << amount
              << (ok ? "    filled" : "    skipped") << std::endl;
    return ok;
}
      

void Wallet::processSale(OrderBookEntry& sale)
{
    std::vector<std::string> currs = CSVReader::tokenise(sale.product, '/');
    if (currs.size() != 2) return;   // malformed product: leave balances untouched
    // ask
    if (sale.orderType == OrderBookType::asksale)
    {
        double outgoingAmount = sale.amount;
        std::string outgoingCurrency = currs[0];
        double incomingAmount = sale.amount * sale.price;
        std::string incomingCurrency = currs[1];

        currencies[incomingCurrency] += incomingAmount;
        currencies[outgoingCurrency] -= outgoingAmount;

    }
    // bid
    if (sale.orderType == OrderBookType::bidsale)
    {
        double incomingAmount = sale.amount;
        std::string incomingCurrency = currs[0];
        double outgoingAmount = sale.amount * sale.price;
        std::string outgoingCurrency = currs[1];

        currencies[incomingCurrency] += incomingAmount;
        currencies[outgoingCurrency] -= outgoingAmount;
    }
}
void Wallet::loadFromFile(const std::string& username)
{
    currencies.clear();
    std::ifstream file{walletFile};
    std::string line;
    while (std::getline(file, line))
    {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() != 3 || tokens[0] != username) continue;
        try
        {
            currencies[tokens[1]] = std::stod(tokens[2]);
        }
        catch (const std::exception& e) { }   // skip malformed rows
    }
}

void Wallet::saveToFile(const std::string& username) const
{
    // the file holds every user's balances — keep everyone else's rows untouched,
    // then replace this user's rows with the current in-memory map
    std::vector<std::string> otherLines;
    std::ifstream in{walletFile};
    std::string line;
    while (std::getline(in, line))
    {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() == 3 && tokens[0] != username)
        {
            otherLines.push_back(line);
        }
    }

    std::ofstream out{walletFile, std::ios::trunc};
    for (const std::string& l : otherLines) out << l << std::endl;
    for (std::pair<const std::string, double> const& c : currencies)
    {
        out << username << "," << c.first << "," << c.second << std::endl;
    }
}

std::ostream& operator<<(std::ostream& os,  Wallet& wallet)
{
    os << wallet.toString();
    return os;
}
