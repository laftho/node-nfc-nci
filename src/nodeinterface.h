#ifndef NODEINTERFACE_H
#define NODEINTERFACE_H

#include <string>
#include "napi.h"
#include "tagmanager.h"

class NodeInterface: public ITagManager
{
private:
  Napi::Env* env;
  Napi::Env* listenEnv;
  Napi::Function* listenCallback;
  Napi::Function errorCallback;
  Napi::Function tagArrivedCallback;
  Napi::Function tagDepartedCallback;
public:
  NodeInterface(Napi::Env* env, Napi::Function* callback);
  ~NodeInterface();
  
  void on(const Napi::CallbackInfo& info);
  
  void onTagArrived(Tag tag);
  void onTagDeparted();
  void onError(std::string message);
};

#endif // NODEINTERFACE_H
