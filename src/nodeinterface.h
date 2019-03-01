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
public:
  Tag::Tag* tag;
  std::string error;

  NodeInterface(Napi::Env* env, Napi::Function* callback);
  ~NodeInterface();

  void write(const Napi::CallbackInfo& info);
  Napi::Object asNapiObjectTag(Napi::Env* env, Tag::Tag tag);

  void handleOnTagArrived(Napi::Env* env, Napi::FunctionReference* func);
  void handleOnTagDeparted(Napi::Env* env, Napi::FunctionReference* func);
  void handleOnTagWritten(Napi::Env* env, Napi::FunctionReference* func);
  void handleOnError(Napi::Env* env, Napi::FunctionReference* func);
  void onTagArrived(Tag::Tag tag);
  void onTagDeparted();
  void onTagWritten(Tag::Tag tag);
  void onError(std::string message);
};

extern NodeInterface* nodei;

class Event: public Napi::AsyncWorker {
private:
  Mutex* mutex;
  std::function<void(Napi::Env*, Napi::FunctionReference*)> handler;
public:
  Event(Napi::Function& callback, Mutex* mutex, std::function<void(Napi::Env*, Napi::FunctionReference*)> handler)
          : Napi::AsyncWorker(callback), mutex(mutex), handler(handler) {}

  ~Event() {}
  void Execute() {
    mutex->Wait(true);
  }

  void OnOk() {
    Napi::Env env = Env();
    Napi::HandleScope scope(env);

    handler(&env, &Callback());

    // Napi::HandleScope scope(Env());

    // Callback().Call({ Env().Null() });

    // Napi::Object tagInfo = nodei->asNapiObjectTag(*nodei->tag);

    // Callback().Call({ Napi::String::New(Env(), "arrived"), tagInfo });
    // nodei->pOnTagArrived();
  }
};

#endif // NODE_NFC_NCI_NODEINTERFACE_H
