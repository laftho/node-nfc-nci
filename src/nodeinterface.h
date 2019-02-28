#ifndef NODE_NFC_NCI_NODEINTERFACE_H
#define NODE_NFC_NCI_NODEINTERFACE_H

#include <string>
#include <exception>
#include "napi.h"
#include "tagmanager.h"
#include "tag.h"
#include "mutex.h"

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
  Tag::Tag* tag;
public:
  NodeInterface(Napi::Env* env, Napi::Function* callback);
  ~NodeInterface();

  void write(const Napi::CallbackInfo& info);
  Napi::Object asNapiObjectTag(Tag::Tag tag);

  void pOnTagArrived();
  void onTagArrived(Tag::Tag tag);
  void onTagDeparted();
  void onTagWritten(Tag::Tag tag);
  void onError(std::string message);
};

extern NodeInterface* nodei;

class Event: public Napi::AsyncWorker {
private:
  Mutex* mutex;
  NodeInterface* nodei;
public:
  Event(Napi::Function& callback, Mutex* mutex, NodeInterface* nodei)
          : Napi::AsyncWorker(callback), mutex(mutex), nodei(nodei) {}

  ~Event() {}
  void Execute() {
    mutex->Wait(true);
  }

  void OnOk() {
    Napi::HandleScope scope(Env());
    // Callback().Call({ Env().Null() });
    nodei->pOnTagArrived();
  }
};

#endif // NODE_NFC_NCI_NODEINTERFACE_H
