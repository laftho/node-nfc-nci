/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <sstream>
#include <iomanip>

#include <unistd.h>

#include <linux_nfc_api.h>

#include "tagmanager.h"
#include "device.h"

TagNDEF::TagNDEF()
{
  size = 0;
  length = 0;
  writeable = false;
}


TagManager& TagManager::getInstance()
{
  static TagManager instance;
  
  return instance;
}

TagManager::TagManager() { }

TagManager::~TagManager() {
  Device::deinitialize();
}

void TagManager::initialize(ITagManager& tagInterface)
{
  this->tagInterface = &tagInterface;
  
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

void TagManager::onTagArrival(nfc_tag_info_t* pTagInfo)
{
  nfc_tag_info_t tagInfo;
  
  memcpy(&tagInfo, pTagInfo, sizeof(nfc_tag_info_t));
  
  Tag tag;
  
  tag.technology = TagTechnology();
  tag.technology.code = tagInfo.technology;
  
  switch(tag.technology.code) {
    case TARGET_TYPE_ISO14443_3A:
    {
      tag.technology.name = "Type A";
      tag.technology.type = "ISO14443_3A";
    } break;
    case TARGET_TYPE_ISO14443_3B:
    {
      tag.technology.name = "Type 4B";
      tag.technology.type = "ISO14443_3B";
    } break;
    case TARGET_TYPE_ISO14443_4:
    {
      tag.technology.name = "Type 4A";
      tag.technology.type = "ISO14443_4";
    } break;
    case TARGET_TYPE_FELICA:
    {
      tag.technology.name = "Type F";
      tag.technology.type = "FELICA";
    } break;
    case TARGET_TYPE_ISO15693:
    {
      tag.technology.name = "Type V";
      tag.technology.type = "ISO15693";
    } break;
    case TARGET_TYPE_NDEF:
    {
      tag.technology.name = "NDEF";
      tag.technology.type = "NDEF";
    } break;
    case TARGET_TYPE_NDEF_FORMATABLE:
    {
      tag.technology.name = "Formatable";
      tag.technology.type = "NDEF_FORMATABLE";
    } break;
    case TARGET_TYPE_MIFARE_CLASSIC:
    {
      tag.technology.name = "Type A - Mifare Classic";
      tag.technology.type = "MIFARE_CLASSIC";
    } break;
    case TARGET_TYPE_MIFARE_UL:
    {
      tag.technology.name = "Type A - Mifare Ul";
      tag.technology.type = "MIFARE_UL";
    } break;
    case TARGET_TYPE_KOVIO_BARCODE:
    {
      tag.technology.name = "Type A - Kovio Barcode";
      tag.technology.type = "KOVIO_BARCODE";
    } break;
    case TARGET_TYPE_ISO14443_3A_3B:
    {
      tag.technology.name = "Type A/B";
      tag.technology.type = "ISO14443_3A_3B";
    } break;
    default:
    {
      tag.technology.name = "Unknown or not supported";
      tag.technology.type = "UNKNOWN";
    } break;
  }
  
  tag.uid = TagUid();
  tag.uid.length = tagInfo.uid_length;
  
  if (tag.uid.length != 0x00 && tag.uid.length <= 32)
  {
    switch(tag.uid.length) {
      case 4:
      case 7:
      case 10:
        tag.uid.type = "NFCID1";
        break;
      case 8:
        tag.uid.type = "NFCID2";
        break;
      default:
        tag.uid.type = "UID";
        break;
    }
    
    std::ostringstream oss;
    oss << std::setfill('0');
    
    for(unsigned int i = 0x00; i < tagInfo.uid_length; i++)
    {
        oss << std::setw(2) << std::hex << static_cast< int >( tagInfo.uid[i] );
        
        if (i < tagInfo.uid_length - 1) {
            oss << ":";
        }
    }
    
    tag.uid.id.assign(oss.str());
  }
  
  ndef_info_t ndefInfo;
  
  unsigned int res = 0x00;
  /*
  res = nfcTag_isNdef(tagInfo.handle, &ndefInfo);
  
  if (res == 0x01) {
    
    tag.ndef = TagNDEF();
    
    if (ndefInfo.is_ndef) {
      tag.ndef.size = ndefInfo.max_ndef_length;
      tag.ndef.length = ndefInfo.current_ndef_length;
      tag.ndef.writeable = (bool)ndefInfo.is_writable;
      
      unsigned char* ndefContent = (unsigned char*)malloc(ndefInfo.current_ndef_length * sizeof(unsigned char));
      
      nfc_friendly_type_t lNDEFType = NDEF_FRIENDLY_TYPE_OTHER;
      
      res = nfcTag_readNdef(tagInfo.handle, ndefContent, ndefInfo.current_ndef_length, &lNDEFType);
      
      if (res != ndefInfo.current_ndef_length)
      {
        // TODO failed to read all the data we requested.
      } else {
        switch(lNDEFType)
        {
          case NDEF_FRIENDLY_TYPE_TEXT:
          {
            char* content = NULL;
            
            content = (char*)malloc(res * sizeof(char));
            res = ndef_readText(ndefContent, res, content, res);
            
            if (res >= 0x00)
            {
              tag.ndef.type = "Text";
              tag.ndef.content = content;
            } else {
              // TODO Read NDEF Text Error
            }
            
            if (content != NULL)
            {
              free(content);
              content = NULL;
            }
          } break;
          case NDEF_FRIENDLY_TYPE_URL:
          {
            char* content = NULL;
            
            content = (char*)malloc(res * sizeof(char));
            res = ndef_readUrl(ndefContent, res, content, res);
            
            if (res >= 0x00)
            {
              tag.ndef.type = "URI";
              tag.ndef.content = content;
            } else {
              // TODO Read NDEF Text Error
            }
            
            if (content != NULL)
            {
              free(content);
              content = NULL;
            }
          } break;
          case NDEF_FRIENDLY_TYPE_HR:
          {
            tag.ndef.type = "Handover Request";
          } break;
          case NDEF_FRIENDLY_TYPE_HS:
          {
            tag.ndef.type = "Handover Select";
          } break;
          case NDEF_FRIENDLY_TYPE_OTHER:
          {
            tag.ndef.type = "OTHER";
          } break;
          default:
          {
            tag.ndef.type = "UNKNOWN";
          } break;
        }
      }
      
      if (ndefContent != NULL)
      {
        free(ndefContent);
        ndefContent = NULL;
      }
    }
  }*/
  
  
  tagInterface->onTag(tag);
}

void TagManager::onTagDeparture()
{
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
