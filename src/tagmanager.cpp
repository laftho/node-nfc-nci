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

// #include "mutex.h"

NFCManagerInitializationException nfcManagerInitializationException;
NFCSNEPServerStartException nfcSNEPServerStartException;
NFCSNEPClientRegisterException nfcSNEPClientRegisterException;

TagNDEF::TagNDEF()
{
  size = 0;
  length = 0;
  writeable = false;
}

void deviceOnTagArrival(nfc_tag_info_t* pTagInfo)
{
  TagManager::getInstance().onTagArrival(pTagInfo);
}

void deviceOnTagDeparture(void)
{
  TagManager::getInstance().onTagDeparture();
}

void deviceOnSnepClientReady()
{
  TagManager::getInstance().onSnepClientReady();
}

void deviceOnSnepClientClosed()
{
  TagManager::getInstance().onSnepClientClosed();
}

void deviceOnDeviceArrival(void)
{
  TagManager::getInstance().onDeviceArrival();
}

void deviceOnDeviceDeparture(void)
{
  TagManager::getInstance().onDeviceDeparture();
}

void deviceOnMessageReceived(unsigned char *message, unsigned int length)
{
  TagManager::getInstance().onMessageReceived(message, length);
}

TagManager& TagManager::getInstance()
{
  static TagManager instance;
  
  return instance;
}

TagManager::TagManager()
{
  // deviceMutex = new Mutex();
  
  tagCallback.onTagArrival = deviceOnTagArrival;
  tagCallback.onTagDeparture = deviceOnTagDeparture;
  
  snepClientCallback.onDeviceArrival = deviceOnSnepClientReady;
  snepClientCallback.onDeviceDeparture = deviceOnSnepClientClosed;
  
  snepServerCallback.onDeviceArrival = deviceOnDeviceArrival;
  snepServerCallback.onDeviceDeparture = deviceOnDeviceDeparture;
  snepServerCallback.onMessageReceived = deviceOnMessageReceived;
}

TagManager::~TagManager()
{
  
  nfcSnep_stopServer();
  
  nfcManager_disableDiscovery();
  
  nfcSnep_deregisterClientCallback();
  
  nfcManager_deregisterTagCallback();
  
  nfcManager_doDeinitialize();
}

void TagManager::initialize(ITagManager& tagInterface)
{
  this->tagInterface = &tagInterface;
  int res = 0x00;
  
  res = nfcManager_doInitialize();
  
  if (res != 0x00) {
    throw nfcManagerInitializationException;
  }
  
  nfcManager_registerTagCallback(&tagCallback);
  
  res = nfcSnep_registerClientCallback(&snepClientCallback);
  if(0x00 != res)
  {
      throw nfcSNEPClientRegisterException;
  }
  
  nfcManager_enableDiscovery(DEFAULT_NFA_TECH_MASK, 0x00, 0x00, 0);
  
  res = nfcSnep_startServer(&snepServerCallback);
  
  if(res != 0x00)
  {
      throw nfcSNEPServerStartException;
  }
  
  // TODO fixme
  // fix this to be a proper loop and mutex wait lock crap
  //  v8 is crashing saying illegal instruction because it seems proper locking isn't in place
  //  my guess is that the on tag stuff is coming from the NFC_TASK thread and there's missing some
  //  locking stuff that we'd get from the mutex in the example that would work around this
  /*
   * 
#
# Fatal error in HandleScope::HandleScope
# Entering the V8 API without proper locking in place
#


Thread 10 "NFC_TASK" received signal SIGILL, Illegal instruction.
[Switching to Thread 0x72cfd450 (LWP 20789)]
0x01387830 in v8::base::OS::Abort() ()
(gdb) info s
#0  0x01387830 in v8::base::OS::Abort() ()
#1  0x006eedc0 in v8::Utils::ReportApiFailure(char const*, char const*) ()
#2  0x006f0280 in v8::EscapableHandleScope::EscapableHandleScope(v8::Isolate*) ()
#3  0x0070e460 in v8::Function::Call(v8::Local<v8::Context>, v8::Local<v8::Value>, int, v8::Local<v8::Value>*) ()
#4  0x004c7f24 in napi_call_function ()
#5  0x742e6508 in Napi::Function::Call (this=0x7effe37c, recv=0x1d94318, argc=1, args=0x72cfc614) at /home/pi/source/node-nfc-nci/node_modules/node-addon-api/napi-inl.h:1704
#6  0x742e64ac in Napi::Function::Call (this=0x7effe37c, recv=0x1d94318, args=...) at /home/pi/source/node-nfc-nci/node_modules/node-addon-api/napi-inl.h:1695
#7  0x742e5328 in NodeInterface::onTag (this=0x7effe370, tag=...) at /home/pi/source/node-nfc-nci/src/nodeinterface.cpp:59
#8  0x742e8abc in TagManager::onTagArrival (this=0x742fb40c <TagManager::getInstance()::instance>, pTagInfo=0x72cfc978) at /home/pi/source/node-nfc-nci/src/tagmanager.cpp:350
#9  0x742e807c in deviceOnTagArrival (pTagInfo=0x72cfc978) at /home/pi/source/node-nfc-nci/src/tagmanager.cpp:43
#10 0x73e9f860 in nativeNfcTag_onTagArrival(nfc_tag_info_t*) () from /usr/local/lib/libnfc_nci_linux-1.so.0
#11 0x73ea38c0 in NfcTag::createNativeNfcTag(tNFA_ACTIVATED&) () from /usr/local/lib/libnfc_nci_linux-1.so.0
#12 0x73eaafb8 in nfaConnectionCallback(unsigned char, tNFA_CONN_EVT_DATA*) () from /usr/local/lib/libnfc_nci_linux-1.so.0
#13 0x73e509f0 in nfa_dm_notify_activation_status () from /usr/local/lib/libnfc_nci_linux-1.so.0
#14 0x73e35094 in nfa_rw_activate_ntf () from /usr/local/lib/libnfc_nci_linux-1.so.0
#15 0x73e30f9c in nfa_rw_proc_disc_evt () from /usr/local/lib/libnfc_nci_linux-1.so.0
#16 0x73e50f88 in nfa_dm_poll_disc_cback () from /usr/local/lib/libnfc_nci_linux-1.so.0
#17 0x73e4d968 in nfa_dm_disc_notify_activation () from /usr/local/lib/libnfc_nci_linux-1.so.0
#18 0x73e4ce28 in nfa_dm_disc_sm_execute () from /usr/local/lib/libnfc_nci_linux-1.so.0
#19 0x73e66a44 in nfc_ncif_proc_activate () from /usr/local/lib/libnfc_nci_linux-1.so.0
#20 0x73e67fe8 in nfc_ncif_process_event () from /usr/local/lib/libnfc_nci_linux-1.so.0
#21 0x73e64044 in nfc_task () from /usr/local/lib/libnfc_nci_linux-1.so.0
#22 0x73e5b474 in gki_task_entry () from /usr/local/lib/libnfc_nci_linux-1.so.0
#23 0x76d60fc4 in start_thread (arg=0x72cfd450) at pthread_create.c:458
#24 0x76ced038 in ?? () at ../sysdeps/unix/sysv/linux/arm/clone.S:76 from /lib/arm-linux-gnueabihf/libc.so.6
Backtrace stopped: previous frame identical to this frame (corrupt stack?)
   */
  
  while (0x01) {
    sleep(10);
  }
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
