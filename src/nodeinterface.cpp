#include <functional>
#include <exception>
#include "nodeinterface.h"
#include <node_api.h>

NodeInterface* nodei;

pthread_t listenThread;
napi_threadsafe_function emitTSF;

Event* onTagArrivedEvent;
Event* onTagWrittenEvent;
Event* onTagDepartedEvent;
Event* onErrorEvent;

void Listener::OnOK() {
  Napi::HandleScope scope(Env());

  if (!error.empty()) {
    onTagArrivedEvent->Terminate(error);
    onTagDepartedEvent->Terminate(error);
    onTagWrittenEvent->Terminate(error);
    onErrorEvent->Terminate(error);
  }

  Callback().Call({ Env().Null(), Napi::String::New(Env(), error) });
}

void Listener::OnError() {
  Napi::HandleScope scope(Env());

  if (!error.empty()) {
    onTagArrivedEvent->Terminate(error);
    onTagDepartedEvent->Terminate(error);
    onTagWrittenEvent->Terminate(error);
    onErrorEvent->Terminate(error);
  }

  Callback().Call({ Env().Null(), Napi::String::New(Env(), error) });
}

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
  /*
  onErrorEvent->Lock();

  error = message;

  onErrorEvent->Notify(false);

  onErrorEvent->Unlock();
  */
  
  napi_call_threadsafe_function(emitTSF, message, napi_tsfn_blocking);
}

void NodeInterface::handleOnError(Napi::Env *env, Napi::FunctionReference *func, std::string error)
{
  if (!error.empty()) { // fatal
    Napi::String mesg = Napi::String::New(*env, error);

    func->Call({ Napi::String::New(*env, "error"), mesg });

    return;
  }

  Napi::String mesg = Napi::String::New(*env, this->error);

  func->Call({ Napi::String::New(*env, "error"), mesg });

  onErrorEvent->Queue();
}

void NodeInterface::onTagDeparted()
{
  onTagDepartedEvent->Notify(true);
}

void NodeInterface::handleOnTagDeparted(Napi::Env *env, Napi::FunctionReference *func, std::string error)
{
  if (!error.empty()) {
    Napi::String mesg = Napi::String::New(*env, error);

    func->Call({ Napi::String::New(*env, "error"), mesg });

    return;
  }

  func->Call({ Napi::String::New(*env, "departed") });

  // onTagDepartedEvent->Queue();
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

  onTagWrittenEvent->Lock();
  onTagArrivedEvent->Lock();

  this->tag = &tag;

  onTagArrivedEvent->Notify(false);

  onTagArrivedEvent->Unlock();
  onTagWrittenEvent->Unlock();
}

void NodeInterface::handleOnTagArrived(Napi::Env* env, Napi::FunctionReference* func, std::string error)
{
  if (!error.empty()) {
    Napi::String mesg = Napi::String::New(*env, error);

    func->Call({ Napi::String::New(*env, "error"), mesg });

    return;
  }

  Napi::Object tagInfo = asNapiObjectTag(env, *tag);

  func->Call({ Napi::String::New(*env, "arrived"), tagInfo });

  // onTagArrivedEvent->Queue();
}

void NodeInterface::onTagWritten(Tag::Tag tag)
{
  onTagArrivedEvent->Lock();
  onTagWrittenEvent->Lock();

  this->tag = &tag;

  onTagWrittenEvent->Notify(false);
  onTagWrittenEvent->Unlock();
  onTagArrivedEvent->Unlock();
}

void NodeInterface::handleOnTagWritten(Napi::Env *env, Napi::FunctionReference *func, std::string error) {
  if (!error.empty()) {
    Napi::String mesg = Napi::String::New(*env, error);

    func->Call({ Napi::String::New(*env, "error"), mesg });

    return;
  }

  Napi::Object tagInfo = asNapiObjectTag(env, *tag);

  func->Call({ Napi::String::New(*env, "written"), tagInfo });

  onTagWrittenEvent->Queue();
}

void NodeInterface::write(const Napi::CallbackInfo& info) {
  Napi::Object arg = info[0].As<Napi::Object>();

  Tag::TagNDEF ndef;

  ndef.type = (std::string)arg.Get("type").As<Napi::String>();
  ndef.content = (std::string)arg.Get("content").As<Napi::String>();

  TagManager::getInstance().setWrite(ndef);
}

void *runListenThread(void *arg) {
  (void) arg;
  
  napi_acquire_threadsafe_function(emitTSF);
  
  try {
    TagManager::getInstance().listen(nodei);
  } catch(std::exception& e) {
    nodei->onError(e.what());
  }

  
  napi_release_threadsafe_function(emitTSF, napi_tsfn_release);
  
  pthread_exit(NULL);
};

Napi::Object listen(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  Napi::Function emit = info[0].As<Napi::Function>();
  
  nodei = new NodeInterface(&env, &emit);
  
  pthread_create(&listenThread, NULL, runListenThread, NULL);
  
  /*
  auto tagArrivedHandler = std::bind(&NodeInterface::handleOnTagArrived, nodei, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  onTagArrivedEvent = new Event(emit, tagArrivedHandler);
  onTagArrivedEvent->Queue();

  auto tagWrittenHandler = std::bind(&NodeInterface::handleOnTagWritten, nodei, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  onTagWrittenEvent = new Event(emit, tagWrittenHandler);
  onTagWrittenEvent->Queue();

  auto tagDepartedHandler = std::bind(&NodeInterface::handleOnTagDeparted, nodei, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  onTagDepartedEvent = new Event(emit, tagDepartedHandler);
  onTagDepartedEvent->Queue();

  auto errorHandler = std::bind(&NodeInterface::handleOnError, nodei, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  onErrorEvent = new Event(emit, errorHandler);
  // onErrorEvent->Queue();

  Listener* listener = new Listener(finish, nodei);

  listener->Queue();
*/
  
  
  
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
