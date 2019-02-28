#ifndef NODE_NFC_NCI_NODEINTERFACE_H
#define NODE_NFC_NCI_NODEINTERFACE_H

#include <string>
#include "napi.h"
#include "tagmanager.h"
#include "tag.h"

class NodeInterface: public ITagManager
{
private:
  Napi::Env* env;
  Napi::Function* emit;
public:
  NodeInterface(Napi::Env* env, Napi::Function* callback);
  ~NodeInterface();

  void write(const Napi::CallbackInfo& info);
  Napi::Object asNapiObjectTag(Tag::Tag tag);

  void onTagArrived(Tag::Tag tag);
  void onTagDeparted();
  void onTagWritten(Tag::Tag tag);
  void onError(std::string message);
};

#endif // NODE_NFC_NCI_NODEINTERFACE_H
