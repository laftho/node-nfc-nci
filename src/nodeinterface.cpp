#include <functional>
#include <exception>
#include "nodeinterface.h"

#include <node_api.h>

NodeInterface* nodei;

pthread_t listenThread;
napi_threadsafe_function handleOnTagArrivedTSF;
napi_threadsafe_function handleOnTagDepartedTSF;
napi_threadsafe_function handleOnTagWrittenTSF;
napi_threadsafe_function handleOnErrorTSF;

std::string errorMessage;
Tag::Tag* arrivedTag;
Tag::Tag* writtenTag;

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
  std::string msg = message;

  errorMessage = message;

  napi_call_threadsafe_function(handleOnErrorTSF, &msg, napi_tsfn_nonblocking);
}

static void handleOnError(Napi::Env env, napi_value func, void* context, void* data)
{
  std::string message = errorMessage;

  Napi::String mesg = Napi::String::New(env, message);

  napi_value argv[2] = { Napi::String::New(env, "error"), mesg };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
}

void NodeInterface::onTagDeparted()
{
  napi_call_threadsafe_function(handleOnTagDepartedTSF, NULL, napi_tsfn_nonblocking);
}

static void handleOnTagDeparted(Napi::Env env, napi_value func, void* context, void* data)
{
  napi_value argv[2] = { Napi::String::New(env, "departed") };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
}

Napi::Object asNapiObjectTag(Napi::Env* env, Tag::Tag* tag)
{
  Napi::Object tagInfo = Napi::Object::New(*env);

  Napi::Object technology = Napi::Object::New(*env);
  Napi::Object uid = Napi::Object::New(*env);
  Napi::Object ndef = Napi::Object::New(*env);

  // Napi::Object ndefWritten = Napi::Object::New(*env);

  technology.Set("code", tag->technology.code);
  technology.Set("name", tag->technology.name);
  technology.Set("type", tag->technology.type);
  tagInfo.Set("technology", technology);

  uid.Set("id", tag->uid.id);
  uid.Set("type", tag->uid.type);
  uid.Set("length", tag->uid.length);
  tagInfo.Set("uid", uid);

  ndef.Set("size", tag->ndef.size);
  ndef.Set("length", tag->ndef.length);
  ndef.Set("read", tag->ndef.read);
  ndef.Set("writeable", tag->ndef.writeable);
  ndef.Set("type", tag->ndef.type);
  ndef.Set("content", tag->ndef.content);
  tagInfo.Set("ndef", ndef);

  return tagInfo;
}

void NodeInterface::onTagArrived(Tag::Tag tag)
{
  this->tag = &tag;
  arrivedTag = &tag;

  napi_call_threadsafe_function(handleOnErrorTSF, &arrivedTag, napi_tsfn_nonblocking);
}

static void handleOnTagArrived(Napi::Env env, napi_value func, void* context, void* data)
{
  Napi::Object tagInfo = asNapiObjectTag(&env, arrivedTag);

  napi_value argv[2] = { Napi::String::New(env, "arrived"), tagInfo };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
}

void NodeInterface::onTagWritten(Tag::Tag tag)
{
  this->tag = &tag;
  writtenTag = &tag;

  napi_call_threadsafe_function(handleOnErrorTSF, &writtenTag, napi_tsfn_nonblocking);
}

static void handleOnTagWritten(Napi::Env env, napi_value func, void* context, void* data)
{
  Napi::Object tagInfo = asNapiObjectTag(&env, writtenTag);

  napi_value argv[2] = { Napi::String::New(env, "written"), tagInfo };

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_call_function(env, undefined, func, 2, argv, NULL);
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

Napi::Object listen(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  Napi::Function emit = info[0].As<Napi::Function>();
  
  nodei = new NodeInterface(&env, &emit);

  napi_create_threadsafe_function(env, emit, NULL, Napi::String::New(env, "onTagArrived"), 0, 1, NULL, NULL, NULL, (napi_threadsafe_function_call_js)handleOnTagArrived, &handleOnTagArrivedTSF);
  napi_create_threadsafe_function(env, emit, NULL, Napi::String::New(env, "onTagDeparted"), 0, 1, NULL, NULL, NULL, (napi_threadsafe_function_call_js)handleOnTagDeparted, &handleOnTagDepartedTSF);
  napi_create_threadsafe_function(env, emit, NULL, Napi::String::New(env, "onTagWritten"), 0, 1, NULL, NULL, NULL, (napi_threadsafe_function_call_js)handleOnTagWritten, &handleOnTagWrittenTSF);

  napi_create_threadsafe_function(
          env, //napi_env env,
          emit, //napi_value func,
          NULL, // napi_value async_resource,
          Napi::String::New(env, "onError"), //napi_value async_resource_name,
          0, // size_t max_queue_size,
          1, //size_t initial_thread_count,
          NULL, // void* thread_finalize_data,
          NULL, // napi_finalize thread_finalize_cb,
          NULL, // void* context,
          (napi_threadsafe_function_call_js)handleOnError, // napi_threadsafe_function_call_js call_js_cb,
          &handleOnErrorTSF); // napi_threadsafe_function* result);
  
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
