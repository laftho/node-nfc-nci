#include <string.h>
#include "device.h"

namespace Device {
  State state = State::NONE;
  Mutex mutex;
  nfc_tag_info_t tagInfo;
  
  nfcTagCallback_t tagCallback;
  nfcSnepClientCallback_t snepClientCallback;
  nfcSnepServerCallback_t snepServerCallback;
  
  NFCManagerInitializationException nfcManagerInitializationException;
  NFCSNEPClientRegisterException nfcSNEPClientRegisterException;
  NFCSNEPServerStartException nfcSNEPServerStartException;
  
  void initialize() {
    mutex = Mutex();
    tagCallback.onTagArrival = onTagArrival;
    tagCallback.onTagDeparture = onTagDeparture;
    
    snepClientCallback.onDeviceArrival = onSnepClientReady;
    snepClientCallback.onDeviceDeparture = onSnepClientClosed;
    
    snepServerCallback.onDeviceArrival = onDeviceArrival;
    snepServerCallback.onDeviceDeparture = onDeviceDeparture;
    snepServerCallback.onMessageReceived = onMessageReceived;
    
    int res = 0x00;
  
    res = nfcManager_doInitialize();
    
    if (res != 0x00) {
      throw nfcManagerInitializationException;
    }
    
    nfcManager_registerTagCallback(&tagCallback);
    
    res = nfcSnep_registerClientCallback(&snepClientCallback);
    if(0x00 != res) {
      throw nfcSNEPClientRegisterException;
    }
    
    nfcManager_enableDiscovery(DEFAULT_NFA_TECH_MASK, 0x00, 0x00, 0);
    
    res = nfcSnep_startServer(&snepServerCallback);
    
    if(res != 0x00) {
      throw nfcSNEPServerStartException;
    }
  }
  
  void deinitialize() {
    nfcSnep_stopServer();
    nfcManager_disableDiscovery();
    nfcSnep_deregisterClientCallback();
    nfcManager_deregisterTagCallback();
    nfcManager_doDeinitialize();
  }

  void onTagArrival(nfc_tag_info_t* pTagInfo) {
    mutex.Lock();
    
    if (state == State::WAITING) {
      memcpy(&tagInfo, pTagInfo, sizeof(nfc_tag_info_t));
      state = State::TAG_ARRIVED;
      mutex.Notify(false);
    }
    
    mutex.Unlock();
  }
  
  void onTagDeparture(void) {
    mutex.Lock();

    if (state == State::WAITING) {
      state = State::TAG_DEPARTED;
      mutex.Notify(false);
    }

    mutex.Unlock();
  }

  void onSnepClientReady() {}
  void onSnepClientClosed() {}
  void onDeviceArrival(void) {}
  void onDeviceDeparture(void) {}
  void onMessageReceived(unsigned char *message, unsigned int length) {}
}
