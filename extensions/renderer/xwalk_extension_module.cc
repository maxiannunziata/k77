// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/extensions/renderer/xwalk_extension_module.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "content/public/renderer/v8_value_converter.h"
#include "third_party/blink/public/web/web_frame.h"
#include "xwalk/extensions/renderer/xwalk_module_system.h"
#include "xwalk/extensions/renderer/xwalk_v8_utils.h"

namespace xwalk {
namespace extensions {

namespace {

// This is the key used in the data object passed to our callbacks to store a
// pointer back to XWalkExtensionModule.
const char* kXWalkExtensionModule = "kXWalkExtensionModule";

}  // namespace

XWalkExtensionModule::XWalkExtensionModule(XWalkExtensionClient* client,
                                           XWalkModuleSystem* module_system,
                                           const std::string& extension_name,
                                           const std::string& extension_code)
    : extension_name_(extension_name),
      extension_code_(extension_code),
      converter_(content::V8ValueConverter::Create()),
      client_(client),
      module_system_(module_system),
      instance_id_(0) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Object> function_data = v8::Object::New(isolate);
  function_data->Set(context,v8::String::NewFromUtf8(isolate, kXWalkExtensionModule).ToLocalChecked(),
                     v8::External::New(isolate, this)).Check();

  v8::Handle<v8::ObjectTemplate> object_template =
      v8::ObjectTemplate::New(isolate);
  // TODO(cmarcelo): Use Template::Set() function that takes isolate, once we
  // update the Chromium (and V8) version.
  object_template->Set(context,
      v8::String::NewFromUtf8(isolate, "postMessage").ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, PostMessageCallback, function_data));
  object_template->Set(
      v8::String::NewFromUtf8(isolate, "sendSyncMessage"),
      v8::FunctionTemplate::New(
          isolate, SendSyncMessageCallback, function_data));
  object_template->Set(
      v8::String::NewFromUtf8(isolate, "setMessageListener"),
      v8::FunctionTemplate::New(
          isolate, SetMessageListenerCallback, function_data));

  function_data_.Reset(isolate, function_data);
  object_template_.Reset(isolate, object_template);
}

XWalkExtensionModule::~XWalkExtensionModule() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  // Deleting the data will disable the functions, they'll return early. We do
  // this because it might be the case that the JS objects we created outlive
  // this object (getting references from inside an iframe and then destroying
  // the iframe), even if we destroy the references we have.
  v8::Handle<v8::Object> function_data =
      v8::Local<v8::Object>::New(isolate, function_data_);
  function_data->Delete(v8::String::NewFromUtf8(isolate,
                                                kXWalkExtensionModule));

  object_template_.Reset();
  function_data_.Reset();
  message_listener_.Reset();

  if (instance_id_)
    client_->DestroyInstance(instance_id_);
}

namespace {

std::string CodeToEnsureNamespace(const std::string& extension_name) {
  std::string result;
  size_t pos = 0;
  while (true) {
    pos = extension_name.find('.', pos);
    if (pos == std::string::npos) {
      result += extension_name + " = {};";
      break;
    }
    std::string ns = extension_name.substr(0, pos);
    result += ns + " = " + ns + " || {}; ";
    pos++;
  }
  return result;
}

// Wrap API code into a callable form that takes extension object as parameter.
std::string WrapAPICode(const std::string& extension_code,
                        const std::string& extension_name) {
  // We take care here to make sure that line numbering for api_code after
  // wrapping doesn't change, so that syntax errors point to the correct line.
  return base::StringPrintf(
      "var %s; (function(extension, requireNative) { "
      "extension.internal = {};"
      "extension.internal.sendSyncMessage = extension.sendSyncMessage;"
      "delete extension.sendSyncMessage;"
      "extension.setExports = function(exports){%s = exports;};"
      "(function() {'use strict';"
      "  var exports = {}; %s\n;"
      "extension.setExports(exports);})();});",
      CodeToEnsureNamespace(extension_name).c_str(),
      extension_name.c_str(),
      extension_code.c_str());
}

v8::Handle<v8::Value> RunString(const std::string& code,
                                std::string* exception) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Handle<v8::String> v8_code(
      v8::String::NewFromUtf8(isolate, code.c_str()));

  v8::MicrotasksScope microtasks(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::TryCatch try_catch(isolate);
  try_catch.SetVerbose(true);

  v8::Handle<v8::Script> script(v8::Script::Compile(v8_code));
  if (try_catch.HasCaught()) {
    *exception = ExceptionToString(try_catch);
    return handle_scope.Escape(
        v8::Local<v8::Primitive>(v8::Undefined(isolate)));
  }

  v8::Local<v8::Value> result = script->Run();
  if (try_catch.HasCaught()) {
    *exception = ExceptionToString(try_catch);
    return handle_scope.Escape(
        v8::Local<v8::Primitive>(v8::Undefined(isolate)));
  }

  return handle_scope.Escape(result);
}

}  // namespace

void XWalkExtensionModule::LoadExtensionCode(
    v8::Handle<v8::Context> context, v8::Handle<v8::Function> requireNative) {
  CHECK(!instance_id_);
  instance_id_ = client_->CreateInstance(extension_name_, this);

  std::string exception;
  std::string wrapped_api_code = WrapAPICode(extension_code_, extension_name_);
  v8::Handle<v8::Value> result =
      RunString(wrapped_api_code, &exception);
  if (!result->IsFunction()) {
    LOG(WARNING) << "Couldn't load JS API code for " << extension_name_
      << ": " << exception;
    return;
  }
  v8::Handle<v8::Function> callable_api_code =
      v8::Handle<v8::Function>::Cast(result);
  v8::Handle<v8::ObjectTemplate> object_template =
      v8::Local<v8::ObjectTemplate>::New(context->GetIsolate(),
                                          object_template_);

  const int argc = 2;
  v8::Handle<v8::Value> argv[argc] = {
    object_template->NewInstance(),
    requireNative
  };

  v8::MicrotasksScope microtasks(
      context->GetIsolate(), v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::TryCatch try_catch(context->GetIsolate());
  try_catch.SetVerbose(true);
  callable_api_code->Call(context->Global(), argc, argv);
  if (try_catch.HasCaught()) {
    LOG(WARNING) << "Exception while loading JS API code for "
        << extension_name_ << ": " << ExceptionToString(try_catch);
  }
}

void XWalkExtensionModule::HandleMessageFromNative(const base::Value& msg) {
  if (message_listener_.IsEmpty())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Context> context = module_system_->GetV8Context();
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Value> v8_value(converter_->ToV8Value(&msg, context));
  v8::Handle<v8::Function> message_listener =
      v8::Local<v8::Function>::New(isolate, message_listener_);;

  v8::MicrotasksScope microtasks(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::TryCatch try_catch(isolate);
  message_listener->Call(context->Global(), 1, &v8_value);
  if (try_catch.HasCaught())
    LOG(WARNING) << "Exception when running message listener: "
        << ExceptionToString(try_catch);
}

// static
void XWalkExtensionModule::PostMessageCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::ReturnValue<v8::Value> result(info.GetReturnValue());
  XWalkExtensionModule* module = GetExtensionModule(info);
  if (!module || info.Length() != 1) {
    result.Set(false);
    return;
  }

  v8::Handle<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  std::unique_ptr<base::Value> value(
      module->converter_->FromV8Value(info[0], context));

  CHECK(module->instance_id_);
  module->client_->PostMessageToNative(module->instance_id_, std::move(value));
  result.Set(true);
}

// static
void XWalkExtensionModule::SendSyncMessageCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::ReturnValue<v8::Value> result(info.GetReturnValue());
  XWalkExtensionModule* module = GetExtensionModule(info);
  if (!module || info.Length() != 1) {
    result.Set(false);
    return;
  }

  v8::Handle<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  std::unique_ptr<base::Value> value(
      module->converter_->FromV8Value(info[0], context));

  CHECK(module->instance_id_);
  std::unique_ptr<base::Value> reply(
      module->client_->SendSyncMessageToNative(module->instance_id_,
                                               std::move(value)));

  // If we tried to send a message to an instance that became invalid,
  // then reply will be NULL.
  if (reply)
    result.Set(module->converter_->ToV8Value(reply.get(), context));
}

// static
void XWalkExtensionModule::SetMessageListenerCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::ReturnValue<v8::Value> result(info.GetReturnValue());
  XWalkExtensionModule* module = GetExtensionModule(info);
  if (!module || info.Length() != 1) {
    result.Set(false);
    return;
  }

  if (!info[0]->IsFunction() && !info[0]->IsUndefined()) {
    LOG(WARNING) << "Trying to set message listener with invalid value.";
    result.Set(false);
    return;
  }

  v8::Isolate* isolate = info.GetIsolate();
  if (info[0]->IsUndefined())
    module->message_listener_.Reset();
  else
    module->message_listener_.Reset(isolate, info[0].As<v8::Function>());

  result.Set(true);
}

// static
XWalkExtensionModule* XWalkExtensionModule::GetExtensionModule(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Object> data = info.Data().As<v8::Object>();
  v8::Local<v8::Value> module =
      data->Get(v8::String::NewFromUtf8(isolate, kXWalkExtensionModule));
  if (module.IsEmpty() || module->IsUndefined()) {
    LOG(WARNING) << "Trying to use extension from already destroyed context!";
    return NULL;
  }
  CHECK(module->IsExternal());
  return static_cast<XWalkExtensionModule*>(module.As<v8::External>()->Value());
}

}  // namespace extensions
}  // namespace xwalk
