#include <functional>
#include <exception>
#include <node_api.h>

#include "nodeinterface.h"
#include "tag_serialize.h"

NodeInterface* nodei;

pthread_t listenThread;
napi_threadsafe_function handleOnTagArrivedTSF;
napi_threadsafe_function handleOnTagDepartedTSF;
napi_threadsafe_function handleOnTagWrittenTSF;
napi_threadsafe_function handleOnErrorTSF;

std::string errorMessage;
Tag::Tag* arrivedTag;
Tag::Tag* writtenTag;

NodeInterface::NodeInterface() { }

NodeInterface::~NodeInterface() { }

void NodeInterface::onError(std::string message) {
  std::string msg = message;

  errorMessage = message;

  napi_call_threadsafe_function(handleOnErrorTSF, &msg, napi_tsfn_nonblocking);
}

static void handleOnError(Napi::Env env, napi_value func, void* context, void* data) {
  std::string message = errorMessage;

  Napi::String mesg = Napi::String::New(env, message);

  napi_value argv[2] = { Napi::String::New(env, "error"), mesg };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
}

void NodeInterface::onTagArrived(Tag::Tag* tag) {
  arrivedTag = tag;

  napi_call_threadsafe_function(handleOnTagArrivedTSF, &arrivedTag, napi_tsfn_nonblocking);
}

static void handleOnTagArrived(Napi::Env env, napi_value func, void* context, void* data) {
  Napi::Object tagInfo = TagSerialize::Tag(&env, arrivedTag);
  tagInfo.Set("ndef", TagSerialize::NDEF(&env, arrivedTag->ndef));

  napi_value argv[2] = { Napi::String::New(env, "arrived"), tagInfo };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
}

void NodeInterface::onTagWritten(Tag::Tag* tag) {
  writtenTag = tag;

  napi_call_threadsafe_function(handleOnTagWrittenTSF, &writtenTag, napi_tsfn_nonblocking);
}

static void handleOnTagWritten(Napi::Env env, napi_value func, void* context, void* data) {
  Napi::Object tagInfo = TagSerialize::Tag(&env, writtenTag);
  tagInfo.Set("ndef", TagSerialize::NDEF(&env, writtenTag->ndefWritten->updated));

  Napi::Object previous = TagSerialize::Tag(&env, writtenTag);
  previous.Set("ndef", TagSerialize::NDEF(&env, writtenTag->ndefWritten->previous));

  napi_value argv[3] = { Napi::String::New(env, "written"), tagInfo, previous };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 3, argv, NULL);
}

void NodeInterface::setNextWrite(const Napi::CallbackInfo& info) {
  Napi::Object arg = info[0].As<Napi::Object>();

  Tag::TagNDEF* ndef = new Tag::TagNDEF();

  ndef->type = (std::string)arg.Get("type").As<Napi::String>();
  ndef->content = (std::string)arg.Get("content").As<Napi::String>();

  TagManager::getInstance().setNextWrite(ndef);
}

void NodeInterface::immediateWrite(const Napi::CallbackInfo &info) {
  Napi::Object arg = info[0].As<Napi::Object>();

  Tag::TagNDEF* ndef = new Tag::TagNDEF();

  ndef->type = (std::string)arg.Get("type").As<Napi::String>();
  ndef->content = (std::string)arg.Get("content").As<Napi::String>();

  TagManager::getInstance().immediateWrite(ndef, true);
}

void NodeInterface::clearNextWrite(const Napi::CallbackInfo &info) {
  TagManager::getInstance().clearNextWrite();
}

Napi::Boolean NodeInterface::hasNextWrite(const Napi::CallbackInfo &info) {
  auto has = TagManager::getInstance().hasNextWrite();

  Napi::Env env = info.Env();

  return Napi::Boolean::New(env, has);
}

void NodeInterface::onTagDeparted() {
  napi_call_threadsafe_function(handleOnTagDepartedTSF, NULL, napi_tsfn_nonblocking);
}

static void handleOnTagDeparted(Napi::Env env, napi_value func, void* context, void* data) {
  Napi::Object tagInfo = TagSerialize::Tag(&env, arrivedTag);
  tagInfo.Set("ndef", TagSerialize::NDEF(&env, arrivedTag->ndef));

  napi_value argv[2] = { Napi::String::New(env, "departed"), tagInfo };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
}

void *runListenThread(void *arg) {
  (void) arg;

  napi_acquire_threadsafe_function(handleOnTagArrivedTSF);
  napi_acquire_threadsafe_function(handleOnTagDepartedTSF);
  napi_acquire_threadsafe_function(handleOnTagWrittenTSF);
  napi_acquire_threadsafe_function(handleOnErrorTSF);
  
  try {
    TagManager::getInstance().listen(nodei);
  } catch(std::exception& e) {
    nodei->onError(e.what());
  }

  napi_release_threadsafe_function(handleOnErrorTSF, napi_tsfn_release);
  napi_release_threadsafe_function(handleOnTagWrittenTSF, napi_tsfn_release);
  napi_release_threadsafe_function(handleOnTagDepartedTSF, napi_tsfn_release);
  napi_release_threadsafe_function(handleOnTagArrivedTSF, napi_tsfn_release);
  
  pthread_exit(NULL);
};

Napi::Object listen(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Function emit = info[0].As<Napi::Function>();
  
  nodei = new NodeInterface();

  napi_create_threadsafe_function(
    env, emit, NULL, Napi::String::New(env, "onTagArrived"), 0, 1, NULL, NULL, NULL,
    (napi_threadsafe_function_call_js)handleOnTagArrived, &handleOnTagArrivedTSF
  );

  napi_create_threadsafe_function(
    env, emit, NULL, Napi::String::New(env, "onTagDeparted"), 0, 1, NULL, NULL, NULL,
    (napi_threadsafe_function_call_js)handleOnTagDeparted, &handleOnTagDepartedTSF
  );

  napi_create_threadsafe_function(
    env, emit, NULL, Napi::String::New(env, "onTagWritten"), 0, 1, NULL, NULL, NULL,
    (napi_threadsafe_function_call_js)handleOnTagWritten, &handleOnTagWrittenTSF
  );

  napi_create_threadsafe_function(
    env, emit, NULL, Napi::String::New(env, "onError"), 0, 1, NULL, NULL, NULL,
    (napi_threadsafe_function_call_js)handleOnError, &handleOnErrorTSF
  );
  
  pthread_create(&listenThread, NULL, runListenThread, NULL);

  Napi::Object context = Napi::Object::New(env);

  auto setNextWrite = std::bind(&NodeInterface::setNextWrite, nodei, std::placeholders::_1);
  context.Set("setNextWrite", Napi::Function::New(env, setNextWrite));

  auto clearNextWrite = std::bind(&NodeInterface::clearNextWrite, nodei, std::placeholders::_1);
  context.Set("clearNextWrite", Napi::Function::New(env, clearNextWrite));

  auto hasNextWrite = std::bind(&NodeInterface::hasNextWrite, nodei, std::placeholders::_1);
  context.Set("hasNextWrite", Napi::Function::New(env, hasNextWrite));

  auto immediateWrite = std::bind(&NodeInterface::immediateWrite, nodei, std::placeholders::_1);
  context.Set("immediateWrite", Napi::Function::New(env, immediateWrite));

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
