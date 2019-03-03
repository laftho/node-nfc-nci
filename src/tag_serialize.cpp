#include "tag_serialize.h"

namespace TagSerialize {
  Napi::Object Technology(Napi::Env *env, Tag::TagTechnology *technology) {
    Napi::Object obj = Napi::Object::New(*env);
    obj.Set("code", technology->code);
    obj.Set("name", technology->name);
    obj.Set("type", technology->type);

    return obj;
  }

  Napi::Object UID(Napi::Env *env, Tag::TagUid *uid) {
    Napi::Object obj = Napi::Object::New(*env);

    obj.Set("id", uid->id);
    obj.Set("type", uid->type);
    obj.Set("length", uid->length);

    return obj;
  }

  Napi::Object NDEF(Napi::Env *env, Tag::TagNDEF *ndef) {
    Napi::Object obj = Napi::Object::New(*env);

    obj.Set("size", ndef->size);
    obj.Set("length", ndef->length);
    obj.Set("read", ndef->read);
    obj.Set("writeable", ndef->writeable);
    obj.Set("type", ndef->type);
    obj.Set("content", ndef->content);

    return obj;
  }

  Napi::Object Tag(Napi::Env *env, Tag::Tag *tag) {
    Napi::Object tagInfo = Napi::Object::New(*env);

    Napi::Object technology = Technology(env, tag->technology);
    Napi::Object uid = UID(env, tag->uid);

    tagInfo.Set("technology", technology);
    tagInfo.Set("uid", uid);

    return tagInfo;
  }
}
