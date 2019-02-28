#ifndef NODE_NFC_NCI_TAG_H
#define NODE_NFC_NCI_TAG_H

#include <string>
#include <linux_nfc_api.h>

namespace Tag {
  class TagTechnology {
  public:
    std::string name;
    std::string type;
    unsigned int code;
  };

  class TagUid {
  public:
    std::string id;
    std::string type;
    unsigned int length;
  };

  class TagNDEF {
  public:
    TagNDEF();

    unsigned int size;
    unsigned int length;
    unsigned int read;
    bool writeable;
    std::string type;
    std::string content;
    std::string error;
  };

  class TagNDEFWritten
  {
  public:
    TagNDEF written;
    TagNDEF previous;
    TagNDEF updated;
  };

  class Tag
  {
  public:
    TagTechnology technology;
    TagUid uid;
    TagNDEF ndef;
    TagNDEFWritten ndefWritten;
  };

  Tag readTag(nfc_tag_info_t* tagInfo);
  TagTechnology readTagTechnology(nfc_tag_info_t* tagInfo);
  TagUid readTagUid(nfc_tag_info_t* tagInfo);
  TagNDEF readTagNDEF(nfc_tag_info_t* tagInfo);
  TagNDEF writeTagNdef(nfc_tag_info_t* tagInfo, TagNDEF ndef);
};

#endif //NODE_NFC_NCI_TAG_H
