// START MY CODE
#pragma once
#include <string>

/** Task 1: simple value object for one OHLC candle period (a year or a day). */
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
// END MY CODE
