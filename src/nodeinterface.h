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
    } catch(std::exception& e) {
      error = e.what();
    }
  }

  void OnOK();
  void OnError();
};

Napi::Object asNapiObjectTag(Napi::Env* env, Tag::Tag* tag);

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

  // void handleOnTagArrived(Napi::Env* env, Napi::FunctionReference* func, std::string error);
  // void handleOnTagDeparted(Napi::Env* env, Napi::FunctionReference* func, std::string error);
  // void handleOnTagWritten(Napi::Env* env, Napi::FunctionReference* func, std::string error);
  // void handleOnError(Napi::Env* env, Napi::FunctionReference* func, std::string error);
  // void handleOnError(Napi::Env env, Napi::Function func, void* context, void* data);
  void onTagArrived(Tag::Tag* tag);
  void onTagDeparted();
  void onTagWritten(Tag::Tag* tag);
  void onError(std::string message);
};

extern NodeInterface* nodei;

class Event: public Napi::AsyncWorker {
private:
  Mutex* mutex;
  bool trigger = false;
  std::string perror;
  std::function<void(Napi::Env*, Napi::FunctionReference*, std::string)> handler;
public:
  Event(Napi::Function& callback, std::function<void(Napi::Env*, Napi::FunctionReference*, std::string)> handler)
          : Napi::AsyncWorker(callback), handler(handler) {
    mutex = new Mutex();
  }

  ~Event() {}
  void Execute() {
    do {
      mutex->Wait(true);
    } while(!trigger);
  }

  int Lock() {
    return mutex->Lock();
  }

  int Unlock() {
    return mutex->Unlock();
  }

  void Notify(bool needsLock) {
    trigger = true;
    mutex->Notify(needsLock);
  }

  void Terminate(std::string error) {
    Lock();
    perror = error;
    Notify(false);
    Unlock();
  }

  void OnOK() {
    Napi::Env env = Env();
    Napi::HandleScope scope(env);

    std::string error = perror;
    trigger = false;

    handler(&env, &Callback(), error);
  }

  void OnError(const Napi::Error& e)
  {
    Napi::Env env = Env();
    Napi::HandleScope scope(env);

    trigger = false;

    handler(&env, &Callback(), e.Message());
  }
};

#endif // NODE_NFC_NCI_NODEINTERFACE_H
