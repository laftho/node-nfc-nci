#ifndef NODE_NFC_NCI_TAGMANAGER_H
#define NODE_NFC_NCI_TAGMANAGER_H

#include <string>
#include <exception>
#include <linux_nfc_api.h>
#include "tag.h"

class ITagManager {
public:
  virtual void onTagArrived(Tag::Tag* tag) = 0;
  virtual void onTagDeparted() = 0;
  virtual void onError(std::string message) = 0;
  virtual void onTagWritten(Tag::Tag* tag) = 0;
};

class TagManager
{
private:
  ITagManager* tagInterface;

  bool hasNextWriteNDEF = false;
  Tag::TagNDEF* nextWriteNDEF;

  TagManager();
  
  TagManager(TagManager const&);
  void operator=(TagManager const&);
public:
  ~TagManager();

  void listen(ITagManager* tagInterface);

  void onTagArrival(nfc_tag_info_t *pTagInfo);
  void onTagDeparture(void);

  void immediateWrite(Tag::TagNDEF* ndef, bool needsLock);
  void setWrite(Tag::TagNDEF* ndef);
  bool hasWrite();
  void clearWrite();

  void onDeviceArrival(void);
  void onDeviceDeparture(void);
  void onMessageReceived(unsigned char *message, unsigned int length);
  
  void onSnepClientReady();
  void onSnepClientClosed();
  
  static TagManager& getInstance();
};

#endif // NODE_NFC_NCI_TAGMANAGER_H
