#ifndef NODE_NFC_NCI_NODEINTERFACE_H
#define NODE_NFC_NCI_NODEINTERFACE_H

#include <string>
#include <exception>
#include "napi.h"
#include "tagmanager.h"
#include "tag.h"

class Listener: public Napi::AsyncWorker {
private:
  ITagManager* nodei;
  std::string error;
public:
  Listener(Napi::Function& callback, ITagManager* nodei)
  : Napi::AsyncWorker(callback), nodei(nodei) {}

  ~Listener() {}
  void Execute() {
    try {
      TagManager::getInstance().listen(nodei);
    } catch(std::exception* e) {
      error = e->what();
    }
  }

  void OnOK() {
    Napi::HandleScope scope(Env());
    Callback().Call({ Env().Null(), Napi::String::New(Env(), error) });
  }
};

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

extern NodeInterface* nodei;

#endif // NODE_NFC_NCI_NODEINTERFACE_H
