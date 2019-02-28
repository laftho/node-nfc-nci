#include <functional>
#include <exception>
#include "nodeinterface.h"

NodeInterface::NodeInterface(Napi::Env *env, Napi::Function *callback)
{
  this->env = env;
  this->emit = callback;
}

NodeInterface::~NodeInterface()
{

}

void NodeInterface::onError(std::string message)
{
  Napi::String mesg = Napi::String::New(*env, message);
  
  emit->Call({ Napi::String::New(*env, "error"), mesg });
}

void NodeInterface::onTagDeparted()
{
  emit->Call({ Napi::String::New(*env, "departed") });
}

Napi::Object NodeInterface::asNapiObjectTag(Tag::Tag tag)
{
  Napi::Object tagInfo = Napi::Object::New(*env);

  Napi::Object technology = Napi::Object::New(*env);
  Napi::Object uid = Napi::Object::New(*env);
  Napi::Object ndef = Napi::Object::New(*env);

  // Napi::Object ndefWritten = Napi::Object::New(*env);

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
  ndef.Set("read", tag.ndef.read);
  ndef.Set("writeable", tag.ndef.writeable);
  ndef.Set("type", tag.ndef.type);
  ndef.Set("content", tag.ndef.content);
  tagInfo.Set("ndef", ndef);

  return tagInfo;
}

void NodeInterface::onTagArrived(Tag::Tag tag)
{
  Napi::Object tagInfo = asNapiObjectTag(tag);
  
  emit->Call({ Napi::String::New(*env, "arrived"), tagInfo });
}

void NodeInterface::onTagWritten(Tag::Tag tag)
{
  Napi::Object tagInfo = asNapiObjectTag(tag);

  emit->Call({ Napi::String::New(*env, "written"), tagInfo });
}

void NodeInterface::write(const Napi::CallbackInfo& info) {
  Napi::Object arg = info[0].As<Napi::Object>();

  Tag::TagNDEF ndef;

  ndef.type = (std::string)arg.Get("type").As<Napi::String>();
  ndef.content = (std::string)arg.Get("content").As<Napi::String>();

  TagManager::getInstance().setWrite(ndef);
}

Napi::Object listen(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  Napi::Function emit = info[0].As<Napi::Function>();
  Napi::Function finish = info[1].As<Napi::Function>();
  
  nodei = NodeInterface(&env, &emit);

  // TagManager::getInstance().listen(nodei);

  Listener* listener = new Listener(finish, &nodei);

  listener->Queue();

  Napi::Object context = Napi::Object::New(env);

  auto write = std::bind(&NodeInterface::write, nodei, std::placeholders::_1);

  context.Set("write", Napi::Function::New(env, write));

  return context;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(
    Napi::String::New(env, "listen"),
    Napi::Function::New(env, listen)
  );
  
  return exports;
}

NODE_API_MODULE(node_nfc_nci, Init)
