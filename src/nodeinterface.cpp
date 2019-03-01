#include <functional>
#include <exception>
#include "nodeinterface.h"

NodeInterface* nodei;
Mutex* onTagArrivedMutex;
Event* onTagArrivedEvent;

Mutex* onTagWrittenMutex;
Event* onTagWrittenEvent;

Mutex* onTagDepartedMutex;
Event* onTagDepartedEvent;

Mutex* onErrorMutex;
Event* onErrorEvent;

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
  onErrorMutex->Lock();

  error = message;

  onErrorMutex->Notify(false);

  onErrorMutex->Unlock();
}

void NodeInterface::handleOnError(Napi::Env *env, Napi::FunctionReference *func)
{
  Napi::String mesg = Napi::String::New(*env, error);

  func->Call({ Napi::String::New(*env, "error"), mesg });
}

void NodeInterface::onTagDeparted()
{
  onTagDepartedMutex->Notify(true);
}

void NodeInterface::handleOnTagDeparted(Napi::Env *env, Napi::FunctionReference *func)
{
  func->Call({ Napi::String::New(*env, "departed") });
}

Napi::Object NodeInterface::asNapiObjectTag(Napi::Env* env, Tag::Tag tag)
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
  //Napi::Object tagInfo = asNapiObjectTag(tag);
  
  //emit->Call({ Napi::String::New(*env, "arrived"), tagInfo });

  onTagWrittenMutex->Lock();
  onTagArrivedMutex->Lock();

  this->tag = &tag;

  onTagArrivedMutex->Notify(false);

  onTagArrivedMutex->Unlock();
  onTagWrittenMutex->Unlock();
}

void NodeInterface::handleOnTagArrived(Napi::Env* env, Napi::FunctionReference* func)
{
  Napi::Object tagInfo = asNapiObjectTag(env, *tag);

  func->Call({ Napi::String::New(*env, "arrived"), tagInfo });
}

void NodeInterface::onTagWritten(Tag::Tag tag)
{
  onTagArrivedMutex->Lock();
  onTagWrittenMutex->Lock();

  this->tag = &tag;

  onTagWrittenMutex->Notify(false);
  onTagWrittenMutex->Unlock();
  onTagArrivedMutex->Unlock();
}

void NodeInterface::handleOnTagWritten(Napi::Env *env, Napi::FunctionReference *func) {
  Napi::Object tagInfo = asNapiObjectTag(env, *tag);

  func->Call({ Napi::String::New(*env, "written"), tagInfo });
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
  
  nodei = new NodeInterface(&env, &emit);

  onTagArrivedMutex = new Mutex();
  auto tagArrivedHandler = std::bind(&NodeInterface::handleOnTagArrived, nodei, std::placeholders::_1, std::placeholders::_2);
  onTagArrivedEvent = new Event(emit, onTagArrivedMutex, tagArrivedHandler);
  onTagArrivedEvent->Queue();

  onTagWrittenMutex = new Mutex();
  auto tagWrittenHandler = std::bind(&NodeInterface::handleOnTagWritten, nodei, std::placeholders::_1, std::placeholders::_2);
  onTagWrittenEvent = new Event(emit, onTagWrittenMutex, tagWrittenHandler);
  onTagWrittenEvent->Queue();

  onTagDepartedMutex = new Mutex();
  auto tagDepartedHandler = std::bind(&NodeInterface::handleOnTagDeparted, nodei, std::placeholders::_1, std::placeholders::_2);
  onTagDepartedEvent = new Event(emit, onTagDepartedMutex, tagDepartedHandler);
  onTagDepartedEvent->Queue();

  onErrorMutex = new Mutex();
  auto errorHandler = std::bind(&NodeInterface::handleOnError, nodei, std::placeholders::_1, std::placeholders::_2);
  onErrorEvent = new Event(emit, onErrorMutex, errorHandler);
  // onErrorEvent->Queue();

  // TagManager::getInstance().listen(nodei);

  Listener* listener = new Listener(finish, nodei);

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
