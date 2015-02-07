#define Cef_IRInput_DEBUG
#include "CeF_IRInput.h"

// connect the IR sensor to

int const irSensorPin = 2;

typedef Cef_IRInput::TimingsBuffer < 128 /* buffer size: increase (power of 2) is too many codes are missed */, Cef_IRInput::ParamsLowRes > IrBuffer;
IrBuffer irBuffer;
Cef_IRInput::PinMonitor irPinMonitor;

void isr() // Interrupt Service Handler
{
  irBuffer.interruptHandlerStateChanged( !irPinMonitor.stateIsHigh(), micros() );
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("start");
  if ( !irPinMonitor.begin(irSensorPin, isr) )
    Serial.println("No interrupt can be attached to this pin. Try pin 2 or 3." );
}

Cef_IRInput::Reader irReader;

void loop()
{
  Cef_IRInput::DecodedValue const dv = irReader.read<Cef_IRInput::NEC_Decoder<IrBuffer> >( irBuffer );
  if ( dv.isDecoded() )
    dv.dump();
  // do some otheruseful things here
  delay(50);
}
