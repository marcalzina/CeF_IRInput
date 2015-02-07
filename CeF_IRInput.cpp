#include "Cef_IRInput.h"

using namespace Cef_IRInput;

int PinMonitor::getInterruptId( int pin )
{
  // TO DO: make this section dependant on the type of Arduino.
  switch ( pin )
  {
    case 2: return 0;
    case 3: return 1;
    default:
      return -1;
  }
}

bool PinMonitor::begin( int const pin, void (*interruptHandlerPtr)() )
{
  pinMode( pin, INPUT_PULLUP );
  pinPort = portInputRegister( digitalPinToPort(pin) );
  pinMask = digitalPinToBitMask(pin);
  iInterrupt = getInterruptId(pin);
  if( iInterrupt<0 )
    return false;
  attachInterrupt( iInterrupt, interruptHandlerPtr, CHANGE );
  return true;
}

void PinMonitor::end()
{
  if( 0<=iInterrupt )
    detachInterrupt( iInterrupt );
  iInterrupt = -1;
}
