#ifndef NODE_NFC_NCI_TAG_SERIALIZE_H
#define NODE_NFC_NCI_TAG_SERIALIZE_H

#include "napi.h"
#include "tag.h"

namespace TagSerialize {
  Napi::Object Technology(Napi::Env* env, Tag::TagTechnology* technology);
  Napi::Object UID(Napi::Env* env, Tag::TagUid* uid);
  Napi::Object NDEF(Napi::Env* env, Tag::TagNDEF* ndef);
  Napi::Object Tag(Napi::Env* env, Tag::Tag* tag);
}

#endif //NODE_NFC_NCI_TAG_SERIALIZE_H
