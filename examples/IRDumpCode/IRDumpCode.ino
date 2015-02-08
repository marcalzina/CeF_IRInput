#define Cef_IRInput_DEBUG
// REMOVE the above "#define Cef_IRInput_DEBUG" to get rid of all debug messages and reduce memory usage
#include "CeF_IRInput.h"

// connect the IR sensor/demodulator data pin to irSensorPin
int const irSensorPin = 2; // use 2 or 3 for most Arduinos
Cef_IRInput::StdIrBuffer irBuffer;
Cef_IRInput::PinMonitor irPinMonitor;

void isr() // Interrupt Service Handler
{
  // below: inverse state because the decoder returns 1 when there is no IR
  irBuffer.interruptHandlerStateChanged( !irPinMonitor.stateIsHigh(), micros() );
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello");
  if ( !irPinMonitor.begin(irSensorPin, isr) )
    Serial.println("No interrupt can be attached to this pin. Try pin 2 or 3." );
}

void loop()
{
  Cef_IRInput::DecodedValue const dv = decodeOrClearNEC( irBuffer );
  if ( dv.isDecoded() )
    dv.dump();
  if( dv.is( Cef_IRInput::DecodedValue::NEC_Code, 0xA16EC03F ) )
    Serial.println("You pressed the key that I was expecting!" );
  
  delay(50); // do other useful things here. The longer is will take the bigger you may want to make the buffer (see declaration of Cef_IRInput::StdIrBuffer) so that everything is safely stored until it can be decoded by the next call to decodeOrClearNEC()
}
