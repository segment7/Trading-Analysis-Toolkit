#pragma once
#include <string>

/** One OHLC candle for a period (a year or a day) — Task 1 */
class Candlestick
{
    public:
        Candlestick(std::string _date,
                    double _open, double _high,
                    double _low,  double _close);

        std::string date;   // period label, e.g. "2020" or "2020/06/01"
        double open;        // first price in the period (by time)
        double high;        // highest price in the period
        double low;         // lowest price in the period
        double close;       // last price in the period (by time)
};
