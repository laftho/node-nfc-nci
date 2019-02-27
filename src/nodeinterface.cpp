#include <functional>
#include <exception>
#include "nodeinterface.h"

NodeInterface::NodeInterface(Napi::Env *env, Napi::Function *callback)
{
  this->env = env;
  this->callback = callback;
  
  auto context = Napi::Object::New(*this->env);
  
  auto on = std::bind(&NodeInterface::on, this, std::placeholders::_1);
  
  context.Set("on", Napi::Function::New(*this->env, on));
  
  this->callback->Call(this->env->Global(), { context });
}

NodeInterface::~NodeInterface()
{

}

void NodeInterface::on(const Napi::CallbackInfo& info)
{
  auto event = info[0].As<Napi::String>();
  auto callback = info[1].As<Napi::Function>();
  
  if ((std::string)event == "error") {
    this->errorCallback = callback;
  } else if((std::string)event == "arrived") {
    this->tagArrivedCallback = callback;
  } else if((std::string)event == "departed") {
    this->tagDepartedCallback = callback;
  }
}


void NodeInterface::onError(std::string message)
{
  Napi::String mesg = Napi::String::New(*env, message);
  
  errorCallback.Call(env->Global(), { mesg });
}

void NodeInterface::onTagDeparted()
{
  tagDepartedCallback.Call(env->Global(), {});
}

void NodeInterface::onTagArrived(Tag tag)
{
  Napi::Object tagInfo = Napi::Object::New(*env);
  
  Napi::Object technology = Napi::Object::New(*env);
  Napi::Object uid = Napi::Object::New(*env);
  Napi::Object ndef = Napi::Object::New(*env);
  
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
  
  tagArrivedCallback.Call(env->Global(), { tagInfo });
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
