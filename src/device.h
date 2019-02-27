#ifndef DEVICE_H
#define DEVICE_H

#include <exception>
#include <linux_nfc_api.h>
#include "mutex.h"

namespace Device {
  class NFCManagerInitializationException: public std::exception {
    virtual const char* what() const throw() { return "NFC Manager failed to initialize"; }
  };

  class NFCSNEPClientRegisterException: public std::exception {
    virtual const char* what() const throw() { return "SNEP Client register callbacks failed"; }
  };

  class NFCSNEPServerStartException: public std::exception {
    virtual const char* what() const throw() { return "SNEP Service start failed"; }
  };
  
  enum State {
    NONE,
    WAITING,
    TAG_ARRIVED,
    TAG_DEPARTED
  };
  
  extern State state;
  extern Mutex mutex;
  
  extern nfc_tag_info_t tagInfo;
  
  extern nfcTagCallback_t tagCallback;
  extern nfcSnepClientCallback_t snepClientCallback;
  extern nfcSnepServerCallback_t snepServerCallback;
  
  void initialize();
  void deinitialize();

  void onTagArrival(nfc_tag_info_t* pTagInfo);
  void onTagDeparture(void);
  void onSnepClientReady();
  void onSnepClientClosed();
  void onDeviceArrival(void);
  void onDeviceDeparture(void);
  void onMessageReceived(unsigned char *message, unsigned int length);
}

#endif
