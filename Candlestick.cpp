#include "Candlestick.h"

// Constructor initialisation
Candlestick::Candlestick(std::string _date,
                         double _open, double _high,
                         double _low,  double _close)
: date(_date), 
  open(_open), 
  high(_high), 
  low(_low), 
  close(_close)
{
    
}
