#ifndef pfodRadioDriver_h
#define pfodRadioDriver_h
/**
  pfodRadioDriver for Arduino
  This class abstracts the low level radio functions from pfodParser.
  Implement a sub-class of this to interface pfodSecurity to your particular Radio library
*/
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

class pfodRadioDriver {
  public:
    virtual ~pfodRadioDriver() {}
    virtual bool init() = 0;
    virtual int16_t lastRssi() = 0;
    virtual int getMode() = 0;
    virtual uint8_t getMaxMessageLength() = 0; // it is important to set this correctly
    virtual void setThisAddress(uint8_t addr) = 0;
    virtual bool receive(uint8_t* buf, uint8_t* len) = 0;
    virtual bool send(const uint8_t* data, uint8_t len) = 0;
    virtual uint8_t headerTo() = 0;
    virtual uint8_t headerFrom() = 0;
    virtual uint8_t headerId() = 0;
    virtual uint8_t headerFlags() = 0;
    virtual void setHeaderTo(uint8_t to) = 0;
    virtual void setHeaderFrom(uint8_t from) = 0;
    virtual void setHeaderId(uint8_t id) = 0;
    virtual void setHeaderFlags(uint8_t flags) = 0;
    
    typedef enum {
	Initialising = 0, ///< Transport is initialising. Initial default value until init() is called..
	Sleep,            ///< Transport hardware is in low power sleep mode (if supported)
	Idle,             ///< Transport is idle.
	Tx,               ///< Transport is in the process of transmitting a message.
	Rx,               ///< Transport is in the process of receiving a message.
	Unknown           // some other mode not used by fodRadio
    } RadioMode;

};


#endif // pfodRadioDriver_h
