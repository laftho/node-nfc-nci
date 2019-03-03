#include <string.h>

#include <linux_nfc_api.h>

#include "tagmanager.h"
#include "device.h"

TagManager& TagManager::getInstance()
{
  static TagManager instance;
  
  return instance;
}

TagManager::TagManager() = default;

TagManager::~TagManager() {
  Device::deinitialize();
}

void TagManager::listen(ITagManager* tagInterface)
{
  this->tagInterface = tagInterface;
  
  Device::initialize();
  
  bool running = true;

  while(running) {
    Device::mutex.Lock();

    if (Device::state != Device::State::WAITING) {
      Device::state = Device::State::WAITING;
      Device::mutex.Wait(false);
    }

    switch(Device::state) {
      case Device::State::TAG_ARRIVED: {
        onTagArrival(&Device::tagInfo);
      } break;
      case Device::State::TAG_DEPARTED: {
        onTagDeparture();
      } break;
      default: {
        running = false;
      } break;
    }

    Device::mutex.Unlock();
  };
}

void TagManager::immediateWrite(Tag::TagNDEF* ndef, bool needsLock) {
  if (needsLock) {
    Device::mutex.Lock();
  }

  if (Device::state == Device::State::TAG_ARRIVED) {
    nfc_tag_info_t tagInfo;

    memcpy(&tagInfo, &Device::tagInfo, sizeof(nfc_tag_info_t));
    Tag::Tag* tag = Tag::readTag(&tagInfo);

    tag->ndefWritten = new Tag::TagNDEFWritten();

    tag->ndefWritten->written = Tag::writeTagNDEF(&tagInfo, ndef);
    tag->ndefWritten->previous = tag->ndef;
    tag->ndefWritten->updated = Tag::readTagNDEF(&tagInfo);

    tagInterface->onTagWritten(tag);
  }

  if (needsLock) {
    Device::mutex.Unlock();
  }
}

void TagManager::setWrite(Tag::TagNDEF* ndef) {
  Device::mutex.Lock();

  if (!hasNextWriteNDEF) {
    immediateWrite(ndef, false);
  } else {
    nextWriteNDEF = ndef;
    hasNextWriteNDEF = true;
  }

  Device::mutex.Unlock();
}

Tag::TagNDEF* TagManager::getWrite() {
  return nextWriteNDEF;
}

void TagManager::clearWrite() {
  Device::mutex.Lock();

  nextWriteNDEF = nullptr;
  hasNextWriteNDEF = false;

  Device::mutex.Unlock();
}

void TagManager::onTagArrival(nfc_tag_info_t* pTagInfo)
{
  nfc_tag_info_t tagInfo;
  
  memcpy(&tagInfo, pTagInfo, sizeof(nfc_tag_info_t));
  Tag::Tag* tag = Tag::readTag(&tagInfo);

  tagInterface->onTagArrived(tag);

  if (hasNextWriteNDEF) {
    tag->ndefWritten = new Tag::TagNDEFWritten();

    tag->ndefWritten->written = Tag::writeTagNDEF(&tagInfo, nextWriteNDEF);
    tag->ndefWritten->previous = tag->ndef;
    tag->ndefWritten->updated = Tag::readTagNDEF(&tagInfo);

    hasNextWriteNDEF = false;

    tagInterface->onTagWritten(tag);
  }
}

void TagManager::onTagDeparture()
{
  tagInterface->onTagDeparted();
}

void TagManager::onSnepClientReady()
{
}

void TagManager::onSnepClientClosed()
{
}

void TagManager::onDeviceArrival()
{
}

void TagManager::onDeviceDeparture()
{
}

void TagManager::onMessageReceived(unsigned char* message, unsigned int length)
{
}
