#ifndef INCLUDEGUARD_Cef_IRInput_h
#define INCLUDEGUARD_Cef_IRInput_h

#include <Arduino.h>

namespace Cef_IRInput
{
  struct ParamsLowRes
  {
    typedef uint8_t Ticks;
    static inline Ticks microsToTicks( uint32_t m ) { return 16320<=m ? 255 : (m+32)/64; }
    static inline uint32_t ticksToMicros( Ticks t ) { return t*64; }

    static inline Ticks microsToTicksUnsafe( uint32_t m ) { return (m+32)/64; }
    static inline bool match( Ticks const observed, uint32_t const expected )
    { // we convert expected instead of observed because it is known at compile time and can be optimised
      return microsToTicksUnsafe((expected*3)/4) <= observed && observed <= microsToTicksUnsafe((expected*5)/4);
    }
  };

  struct ParamsHighRes
  {
    typedef uint16_t Ticks;
    static inline Ticks microsToTicks( uint32_t m ) { return min( m, 65535 ); }
    static inline uint32_t ticksToMicros( Ticks t ) { return t; };

    static bool match( Ticks const observed, uint32_t const expected )
    { // we convert expected instead of observed because it is known at compile time and can be optimised
      return (expected*3)/4 <= observed && observed <= (expected*5)/4;
    }
  };

  template< uint16_t bufferSize=128 /* must be a power of 2 */, typename Params_T=ParamsLowRes >
  class TimingsBuffer
  {
  public:
    typedef Params_T Params;
    typedef typename Params::Ticks Ticks;

  private:
    uint16_t itWrite; // next write offset
    uint16_t itRead; // next read offset
    Ticks buffer[bufferSize];
    uint32_t lastWriteTime;

    static inline uint16_t moduloSize( uint16_t const x ) { return x & ( bufferSize-1 ); /* = x % bufferSize because bufferSize is a power of 2 */ }
    static inline uint32_t halfOfMaxDuration() { return 0x80000000; }

  public:
    TimingsBuffer() : lastWriteTime(0), itWrite(0), itRead(0) {}

    /***** recording, via interrupt ******/
    // 2 modes possible:
    // 1) notification of change of state (interrupt handler for a pin state change)
    inline void interruptHandlerStateChanged( bool const stateHigh, uint32_t const now  )
    {
      if( stateHigh == nextWriteIsForHigh() )
        write( 0 ); // we must have missed a transition, so add a dummy one to be in sync
        Ticks const t = Params::microsToTicks( now - lastWriteTime );
      write( t );
      lastWriteTime = now;
    }
    // or polling the state status (interrupt handler for a timer)
    inline void interruptHandlerStateRefresh( bool const stateHigh, uint32_t const now )
    {
      if( stateHigh == nextWriteIsForHigh() )
        return; // still in the same state, do nothing
      write( Params::microsToTicks( now - lastWriteTime ) );
      lastWriteTime = now;
    }

    // getDurationSinceLastWrite() must be called at least once every 35 minutes or a timing could be reported wrong
    // getDurationSinceLastWrite() must be called within noInterrupts() / interrupts() guards otherwise funny things can happen including:
    //   - lastWriteTime could become greater than now if it is updated after micros() is called
    //   - both this function and the interrupt handler could try to write into lastWriteTime simultaneously
    uint32_t getDurationSinceLastWrite()
    {
      uint32_t const now = micros(); // must be after the noInterrupt otherwise  lastWriteTime could become greater than now
      uint32_t durationSinceLastChange = now - lastWriteTime;
      // maintain the difference between now and lastWriteTime no more than half the available range to avoid undetected overflows
      if( halfOfMaxDuration() < durationSinceLastChange )
      {
  #ifdef Cef_IRInput_DEBUG
        Serial.print( "Long idle time: now=" );
        Serial.print( now );
        Serial.print( ", lastWriteTime=" );
        Serial.println( lastWriteTime );
  #endif
        // could use halfOfMaxDuration() instead of halfOfMaxDuration()/2 below but we want to avoid adjusting this all the time, so take a bit of margin
        lastWriteTime = now - halfOfMaxDuration()/2;
        durationSinceLastChange = halfOfMaxDuration()/2;
      }
      return durationSinceLastChange;
    }

    /***** debug *****/
    void dump( uint16_t n ) const // prints to Serial
    {
      Serial.print("size=");
      Serial.print(n);
      char const * lowHighSymbol = nextReadIsForHigh() ? "+-" : "-+" ;
      for(uint16_t i=0; i<n; ++i )
      {
        Serial.print(" ");
        Serial.print( lowHighSymbol[i&1] );
        Serial.print( Params::ticksToMicros( peek(i) ) );
      }
      Serial.println("");
    }

    /***** tools *****/

    inline bool empty() const { return itWrite == itRead; };
    inline uint16_t size() const { return moduloSize( itWrite - itRead ); }
    inline bool nextReadIsForHigh() const { return bool(itRead & 1); }
    inline bool nextWriteIsForHigh() const { return bool(itWrite & 1); }
    inline void write( Ticks const val )
    {
      buffer[itWrite] = val;
      uint16_t const newItWrite = moduloSize(itWrite + 1);
      if ( newItWrite != itRead )
        itWrite = newItWrite;
  #ifdef Cef_IRInput_DEBUG
      else
        Serial.println( "Write overflow" );
  #endif
    }
    inline Ticks peek( uint16_t const i ) const
    {
  #ifdef Cef_IRInput_DEBUG
      if ( size() <= i )
        Serial.println( "Peek overflow" );
  #endif
      return buffer[moduloSize(itRead + i)];
    }
    inline void consume( uint16_t const n )
    {
  #ifdef Cef_IRInput_DEBUG
      if ( size() < n )
        Serial.println( "Consume overflow" );
  #endif
      itRead = moduloSize(itRead + n);
    }
  };

  class PinMonitor
  {
    int iInterrupt;
    volatile uint8_t *pinPort;
    uint8_t pinMask;
    static int getInterruptId( int pin );
  public:

    PinMonitor() : iInterrupt(-1), pinPort(NULL), pinMask(0) {}
    ~PinMonitor() { end(); }
    bool begin( int const pin, void (*interruptHandlerPtr)() ); // returns true if ok, false if this pin has no interrupt handler (try pins 2 or 3)
    void end();

    inline bool stateIsHigh() const { return 0!=( (*pinPort) & pinMask ); }
  };

  class DecodedValue
  {
  public:
    enum EnumType { Empty, NotEnoughData, ParseError, NEC_Code, NEC_Repeat };
    EnumType enumValue;
    uint16_t length; // if NotEnoughData, =min nb of entries - if ParseError, = location of error - otherwise, nb of entries used to decode
    uint32_t numValue;

    inline DecodedValue() : enumValue(Empty) {}
    inline DecodedValue( EnumType ev, uint16_t length ) : enumValue(ev), length(length) {}
    inline DecodedValue( EnumType ev, uint16_t length, uint32_t nv ) : enumValue(ev), length(length), numValue(nv) {}
    inline bool is( EnumType ev, uint32_t nv ) const { return enumValue==ev && numValue==nv; }
    inline bool isEmpty() const { return enumValue==Empty; }
    inline bool isDecoded() const { switch(enumValue) { case Empty: case NotEnoughData: case ParseError: return false; default: return true; } }
    char const * enumToString() const
    {
      switch( enumValue )
      {
        case Empty: return "Empty";
        case NotEnoughData: return "NotEnoughData";
        case ParseError: return "ParseError";
        case NEC_Code: return "NEC Code";
        case NEC_Repeat: return "NEC Repeat";
        default: return "Unknown";
      }
    }
    void dump() const
    {
      Serial.print( enumToString() );
      Serial.print( ", length=" );
      Serial.print( length );
      if( isDecoded() )
      {
        Serial.print( ", value=0x" );
        Serial.println( numValue, HEX );
      }
      else
        Serial.println("");
    }
  };


  class Reader
  {
  public:
    Reader() {}

    static inline uint32_t timeoutMicros() { return 16000; }
  // We assume that HIGH state in TB is pulsed IR, and LOW is no IR
    template<typename Decoder>
    DecodedValue read( typename Decoder::TB& tb )
    {
      noInterrupts();
      uint32_t const durationSinceLastWrite = tb.getDurationSinceLastWrite();
      uint16_t n = tb.size();
      interrupts();
      while( true )
      {
        DecodedValue const dv = Decoder::decode( tb, n );
        if( dv.isDecoded() )
        {
  #ifdef Cef_IRInput_DEBUG
          Serial.print("Parsed code: " );
          tb.dump( dv.length );
  #endif
          tb.consume( dv.length );
          return dv;
        }
        if( dv.enumValue==DecodedValue::NotEnoughData && durationSinceLastWrite<timeoutMicros() )
          return dv;

        // here we should have dv.enumValue==DecodedValue::ParseError or (DecodedValue::NotEnoughData and timeout)
        // drop everything until the next long blank
        typename Decoder::TB::Params::Ticks const minTicks = Decoder::TB::Params::microsToTicks( timeoutMicros() );
        {
          uint16_t i = (tb.nextReadIsForHigh() ? 1 : 2); // check the durations of Low states only, and ignore the first Low state
          for( ; true; i+=2 )
          {
            if( n<=i )
            {
              i=n;
              break;
            }
            if ( minTicks <= tb.peek(i) )
              break;
          }
          if( i )
          {
    #ifdef Cef_IRInput_DEBUG
            Serial.print("Decoder said: " );
            dv.dump();
            Serial.print("Dropped gibberish: " );
            tb.dump( i );
    #endif
            tb.consume(i);
            n -= i;
          }
          else
            return dv;
        }
      }
    }
  };

  template<typename TB_T /* TimingsBuffer */>
  struct NEC_Decoder
  {
  public:
    typedef TB_T TB;
    typedef typename TB::Params::Ticks Ticks;
    static bool isHeaderMark( Ticks t ) { return TB::Params::match( t, 9000 ); }
    static bool isHeaderSpace( Ticks t ) { return TB::Params::match( t, 4500 ); }
    static bool isRepeatSpace( Ticks t ) { return TB::Params::match( t, 2250 ); }
    static bool isBitMark( Ticks t ) { return TB::Params::match( t, 562 ); }
    static bool isOneSpace( Ticks t ) { return TB::Params::match( t, 1687 ); }
    static bool isZeroSpace( Ticks t ) { return TB::Params::match( t, 562 ); }

    static DecodedValue decode( TB const& b, uint16_t const nElems )
    {
      uint16_t offset = b.nextReadIsForHigh() ? 0 : 1; // align properly

      if( nElems<=offset )
        return DecodedValue( DecodedValue::NotEnoughData, offset+1 );

      if( !isHeaderMark( b.peek(offset)) )
        return DecodedValue( DecodedValue::ParseError, offset );

      ++offset;

      if (offset+1<nElems && isRepeatSpace( b.peek(offset) )/* && isBitMark(b.peek(offset+1)) */ )
        return DecodedValue( DecodedValue::NEC_Repeat, offset+2 );

      {
        uint16_t expected = offset + 2 * 32 + 2;
        if( nElems < expected )
          return DecodedValue( DecodedValue::NotEnoughData, expected );
      }

      if (!isHeaderSpace(b.peek(offset)))
        return DecodedValue( DecodedValue::ParseError, offset );

      offset++;

      uint32_t data = 0;
      for (uint16_t i = 0; i < 32; i++)
      {
        if (!isBitMark(b.peek(offset)))
          return DecodedValue( DecodedValue::ParseError, offset );

        offset++;
        if (isOneSpace(b.peek(offset)))
          data = (data << 1) | 1;
        else if (isZeroSpace(b.peek(offset)))
          data <<= 1;
        else
          return DecodedValue( DecodedValue::ParseError, offset );

        offset++;
      }
      /*
      if( !isBitMark(b.peek(offset)) )
        return DecodedValue( DecodedValue::ParseError, offset );
      */
      offset++;
      return DecodedValue( DecodedValue::NEC_Code, offset, data );
    }
  };
} // namespace Cef_IRInput

#endif
