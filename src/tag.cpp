#include <string.h>
#include <sstream>
#include <iomanip>

#include <unistd.h>

#include <linux_nfc_api.h>

#include "tag.h"

namespace Tag {
  TagNDEF::TagNDEF()
  {
    size = 0;
    length = 0;
    writeable = false;
  }

  Tag* readTag(nfc_tag_info_t* tagInfo) {
    Tag* tag = new Tag();

    tag->technology = readTagTechnology(tagInfo);
    tag->uid = readTagUid(tagInfo);
    tag->ndef = readTagNDEF(tagInfo);

    return tag;
  }

  TagTechnology* readTagTechnology(nfc_tag_info_t* tagInfo)
  {
    TagTechnology* technology = new TagTechnology();
    technology->code = tagInfo->technology;

    switch(technology->code) {
      case TARGET_TYPE_ISO14443_3A:
      {
        technology->name = "Type A";
        technology->type = "ISO14443_3A";
      } break;
      case TARGET_TYPE_ISO14443_3B:
      {
        technology->name = "Type 4B";
        technology->type = "ISO14443_3B";
      } break;
      case TARGET_TYPE_ISO14443_4:
      {
        technology->name = "Type 4A";
        technology->type = "ISO14443_4";
      } break;
      case TARGET_TYPE_FELICA:
      {
        technology->name = "Type F";
        technology->type = "FELICA";
      } break;
      case TARGET_TYPE_ISO15693:
      {
        technology->name = "Type V";
        technology->type = "ISO15693";
      } break;
      case TARGET_TYPE_NDEF:
      {
        technology->name = "NDEF";
        technology->type = "NDEF";
      } break;
      case TARGET_TYPE_NDEF_FORMATABLE:
      {
        technology->name = "Formatable";
        technology->type = "NDEF_FORMATABLE";
      } break;
      case TARGET_TYPE_MIFARE_CLASSIC:
      {
        technology->name = "Type A - Mifare Classic";
        technology->type = "MIFARE_CLASSIC";
      } break;
      case TARGET_TYPE_MIFARE_UL:
      {
        technology->name = "Type A - Mifare Ul";
        technology->type = "MIFARE_UL";
      } break;
      case TARGET_TYPE_KOVIO_BARCODE:
      {
        technology->name = "Type A - Kovio Barcode";
        technology->type = "KOVIO_BARCODE";
      } break;
      case TARGET_TYPE_ISO14443_3A_3B:
      {
        technology->name = "Type A/B";
        technology->type = "ISO14443_3A_3B";
      } break;
      default:
      {
        technology->name = "Unknown or not supported";
        technology->type = "UNKNOWN";
      } break;
    }

    return technology;
  }

  TagUid* readTagUid(nfc_tag_info_t* tagInfo) {
    TagUid* uid = new TagUid();

    uid->length = tagInfo->uid_length;

    if (uid->length != 0x00 && uid->length <= 32)
    {
      switch(uid->length) {
        case 4:
        case 7:
        case 10:
          uid->type = "NFCID1";
          break;
        case 8:
          uid->type = "NFCID2";
          break;
        default:
          uid->type = "UID";
          break;
      }

      std::ostringstream oss;
      oss << std::setfill('0');

      for(unsigned int i = 0x00; i < tagInfo->uid_length; i++)
      {
        oss << std::setw(2) << std::hex << static_cast< int >( tagInfo->uid[i] );

        if (i < tagInfo->uid_length - 1) {
          oss << ":";
        }
      }

      uid->id.assign(oss.str());
    }

    return uid;
  }

  TagNDEF* readTagNDEF(nfc_tag_info_t* tagInfo) {
    TagNDEF* ndef = new TagNDEF();

    ndef_info_t ndefInfo;

    unsigned int res = 0x00;

    res = nfcTag_isNdef(tagInfo->handle, &ndefInfo);

    if (res == 0x01) {
      if (ndefInfo.is_ndef) {
        ndef->size = ndefInfo.max_ndef_length;
        ndef->length = ndefInfo.current_ndef_length;
        ndef->writeable = (bool)ndefInfo.is_writable;

        unsigned char* ndefContent = (unsigned char*)malloc(ndefInfo.current_ndef_length * sizeof(unsigned char));

        nfc_friendly_type_t lNDEFType = NDEF_FRIENDLY_TYPE_OTHER;

        res = nfcTag_readNdef(tagInfo->handle, ndefContent, ndefInfo.current_ndef_length, &lNDEFType);

        if (res != ndefInfo.current_ndef_length)
        {
          ndef->error = "NDEF failed to read all data requested";
        } else {
          switch(lNDEFType)
          {
            case NDEF_FRIENDLY_TYPE_TEXT:
            {
              char* content = NULL;

              content = (char*)malloc(res * sizeof(char));
              res = ndef_readText(ndefContent, res, content, res);

              ndef->read = res;
              if (res >= 0x00)
              {
                ndef->type = "Text";
                ndef->content = ((std::string)content).substr(0, res);
              } else {
                ndef->error = "NDEF read text error";
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
              ndef->read = res;

              if (res >= 0x00)
              {
                ndef->type = "URI";
                ndef->content = content;
              } else {
                ndef->error = "NDEF read url error";
              }

              if (content != NULL)
              {
                free(content);
                content = NULL;
              }
            } break;
            case NDEF_FRIENDLY_TYPE_HR:
            {
              ndef->type = "Handover Request";
              ndef->read = 0;
            } break;
            case NDEF_FRIENDLY_TYPE_HS:
            {
              ndef->type = "Handover Select";
              ndef->read = 0;
            } break;
            case NDEF_FRIENDLY_TYPE_OTHER:
            {
              ndef->type = "OTHER";
              ndef->read = 0;
            } break;
            default:
            {
              ndef->type = "UNKNOWN";
              ndef->read = 0;
            } break;
          }
        }

        if (ndefContent != NULL)
        {
          free(ndefContent);
          ndefContent = NULL;
        }
      }
    }

    return ndef;
  }

  TagNDEF* writeTagNDEF(nfc_tag_info_t* tagInfo, TagNDEF* ndef) {
    TagNDEF* wndef = new TagNDEF();

    char* text = (char*)ndef->content.c_str();
    char* lang = (char*)"en";
    unsigned int* len;
    unsigned char** buffer;

    unsigned int size = (unsigned int)(strlen(text) + strlen(lang) + 30); // TODO : replace 30 by TEXT NDEF message header

    len = &size;
    unsigned char* buf = (unsigned char*) malloc(*len * sizeof(unsigned char));

    buffer = &buf;

    wndef->length = *len;
    wndef->size = *len;
    wndef->read = 0;
    wndef->writeable = false;

    int res = ndef_createText(lang, text, *buffer, *len);

    if (res <= 0x00) { // failed
      return wndef;
    }

    wndef->length = (unsigned int)res;
    wndef->size = (unsigned int)res;

    res = nfcTag_writeNdef(tagInfo->handle, *buffer, *len);

    if (res == 0x00) {
      wndef->content = ndef->content;
      wndef->type = ndef->type;
      wndef->writeable = true;
    }

    return wndef;
  }
};
