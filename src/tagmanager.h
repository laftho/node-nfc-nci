#ifndef TAGMANAGER_H
#define TAGMANAGER_H

#include <string>
#include <exception>
#include <linux_nfc_api.h>

class TagTechnology
{
public:
  std::string name;
  std::string type;
  unsigned int code;
};

class TagUid
{
public:
  std::string id;
  std::string type;
  unsigned int length;
};

class TagNDEF
{
public:
  TagNDEF();
  unsigned int size;
  unsigned int length;
  bool writeable;
  std::string type;
  std::string content;
};

class Tag
{
public:
  TagTechnology technology;
  TagUid uid;
  TagNDEF ndef;
};

class ITagManager {
public:
  virtual void onTagArrived(Tag tag) = 0;
  virtual void onTagDeparted() = 0;
  virtual void onError(std::string message) = 0;
};

class TagManager
{
private:
  ITagManager* tagInterface;
  
  TagManager();
  
  TagManager(TagManager const&);
  void operator=(TagManager const&);
public:
  ~TagManager();

  void listen(ITagManager& tagInterface);

  void onTagArrival(nfc_tag_info_t *pTagInfo);
  void onTagDeparture(void);
    
  void onDeviceArrival(void);
  void onDeviceDeparture(void);
  void onMessageReceived(unsigned char *message, unsigned int length);
  
  void onSnepClientReady();
  void onSnepClientClosed();
  
  static TagManager& getInstance();
};

#endif // TAGMANAGER_H
