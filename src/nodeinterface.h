#ifndef NODEINTERFACE_H
#define NODEINTERFACE_H

#include <string>
#include "napi.h"
#include "tagmanager.h"

class NodeInterface: public ITagManager
{
private:
  Napi::Env* env;
  Napi::Function* emit;
public:
  NodeInterface(Napi::Env* env, Napi::Function* callback);
  ~NodeInterface();

  void onTagArrived(Tag tag);
  void onTagDeparted();
  void onError(std::string message);
};

#endif // NODEINTERFACE_H
