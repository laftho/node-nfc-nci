/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// #include "napi.h"
#include "nodeinterface.h"
// #include "tagmanager.h"

NodeInterface::NodeInterface(Napi::Env& env, Napi::Function& callback)
{
  this->env = &env;
  this->callback = &callback;
}

NodeInterface::~NodeInterface()
{

}

void NodeInterface::onTag(Tag tag)
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
    
    callback->Call(env->Global(), { tagInfo });
}


void poll(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  Napi::Function callback = info[0].As<Napi::Function>();
  
  NodeInterface nodei = NodeInterface(env, callback);
  
  TagManager::getInstance().initialize(nodei);
  
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(
    Napi::String::New(env, "poll"),
    Napi::Function::New(env, poll)
  );
  
  return exports;
}

NODE_API_MODULE(node_nfc_nci, Init)
