#ifndef NODEINTERFACE_H
#define NODEINTERFACE_H

#include <string>
#include "napi.h"
#include "tagmanager.h"

class NodeInterface: public ITagManager
{
private:
  Napi::Env* listenEnv;
  Napi::Function* listenCallback;
  
  Napi::Env* errorEnv;
  Napi::Function* errorCallback;
  
  Napi::Env* tagArrivedEnv;
  Napi::Function* tagArrivedCallback;
  
  Napi::Env* tagDepartedEnv;
  Napi::Function* tagDepartedCallback;
public:
  NodeInterface(Napi::Env* env, Napi::Function* callback);
  ~NodeInterface();
  
  void on(const Napi::CallbackInfo& info);
  
  void initOnError(Napi::Env *env, Napi::Function *callback);
  void initOnTagArrived(Napi::Env *env, Napi::Function *callback);
  void initOnTagDeparted(Napi::Env *env, Napi::Function *callback);
  
  void onTagArrived(Tag tag);
  void onTagDeparted();
  void onError(std::string message);
};

#endif // NODEINTERFACE_H
