#include <functional>
#include <exception>
#include "nodeinterface.h"

NodeInterface::NodeInterface(Napi::Env *env, Napi::Function *callback)
{
  this->listenEnv = env;
  this->listenCallback = callback;

  auto context = Napi::Object::New(*this->listenEnv);
  
  auto on = std::bind(&NodeInterface::on, this, std::placeholders::_1);
  
  context.Set("on", Napi::Function::New(*this->listenEnv, on));
  
  this->listenCallback->Call(this->listenEnv->Global(), { context });
}

NodeInterface::~NodeInterface()
{

}

void NodeInterface::on(const Napi::CallbackInfo& info)
{
  auto env = info.Env();
  auto event = info[0].As<Napi::String>();
  auto callback = info[1].As<Napi::Function>();
  
  if ((std::string)event == "error") {
    initOnError(&env, &callback);
  } else if((std::string)event == "arrived") {
    initOnTagArrived(&env, &callback);
  } else if((std::string)event == "departed") {
    initOnTagDeparted(&env, &callback);
  }
}

void NodeInterface::initOnError(Napi::Env *env, Napi::Function *callback)
{
  errorEnv = env;
  errorCallback = callback;
}

void NodeInterface::initOnTagArrived(Napi::Env *env, Napi::Function *callback)
{
  tagArrivedEnv = env;
  tagArrivedCallback = callback;
}

void NodeInterface::initOnTagDeparted(Napi::Env *env, Napi::Function *callback)
{
  tagDepartedEnv = env;
  tagDepartedCallback = callback;
}

void NodeInterface::onError(std::string message)
{
  Napi::String mesg = Napi::String::New(*errorEnv, message);
  
  errorCallback->Call(errorEnv->Global(), { mesg });
}

void NodeInterface::onTagDeparted()
{
  tagDepartedCallback->Call(tagDepartedEnv->Global(), {});
}

void NodeInterface::onTagArrived(Tag tag)
{
  Napi::Object tagInfo = Napi::Object::New(*tagArrivedEnv);
  
  Napi::Object technology = Napi::Object::New(*tagArrivedEnv);
  Napi::Object uid = Napi::Object::New(*tagArrivedEnv);
  Napi::Object ndef = Napi::Object::New(*tagArrivedEnv);
  
  technology.Set("code", tag.technology.code);
  technology.Set("name", tag.technology.name);
  technology.Set("type", tag.technology.type);
  tagInfo.Set("technology", technology);
  
  uid.Set("id", tag.uid.id);
  uid.Set("type", tag.uid.type);
  uid.Set("length", tag.uid.length);
  tagInfo.Set("uid", uid);
  
  ndef.Set("size", tag.ndef.size);
  ndef.Set("length", tag.ndef.length);
  ndef.Set("writeable", tag.ndef.writeable);
  ndef.Set("type", tag.ndef.type);
  ndef.Set("content", tag.ndef.content);
  tagInfo.Set("ndef", ndef);
  
  tagArrivedCallback->Call(tagArrivedEnv->Global(), { tagInfo });
}
void listen(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  Napi::Function callback = info[0].As<Napi::Function>();
  
  NodeInterface nodei = NodeInterface(&env, &callback);
  
  
  
  TagManager::getInstance().listen(nodei);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(
    Napi::String::New(env, "listen"),
    Napi::Function::New(env, listen)
  );
  
  return exports;
}

NODE_API_MODULE(node_nfc_nci, Init)
