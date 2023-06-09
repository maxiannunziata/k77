// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/extensions/renderer/xwalk_module_system.h"

#include <algorithm>
#include "base/command_line.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "v8/include/v8.h"
#include "xwalk/extensions/common/xwalk_extension_switches.h"
#include "xwalk/extensions/renderer/xwalk_extension_client.h"
#include "xwalk/extensions/renderer/xwalk_extension_module.h"

namespace xwalk {
namespace extensions {

namespace {

// Index used to set embedder data into v8::Context, so we can get from a
// context to its corresponding module. Index chosen to not conflict with
// WebCore::V8ContextEmbedderDataField in V8PerContextData.h.
const int kModuleSystemEmbedderDataIndex = 8;

// This is the key used in the data object passed to our callbacks to store a
// pointer back to XWalkExtensionModule.
const char* kXWalkModuleSystem = "kXWalkModuleSystem";

XWalkModuleSystem* GetModuleSystem(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Object> data = info.Data().As<v8::Object>();
  v8::Handle<v8::Value> module_system =
      data->Get(v8::String::NewFromUtf8(isolate, kXWalkModuleSystem));
  if (module_system.IsEmpty() || module_system->IsUndefined()) {
    LOG(WARNING) << "Trying to use requireNative from already "
                 << "destroyed module system!";
    return NULL;
  }

  CHECK(module_system->IsExternal());
  return static_cast<XWalkModuleSystem*>(
      module_system.As<v8::External>()->Value());
}

void RequireNativeCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::ReturnValue<v8::Value> result(info.GetReturnValue());
  XWalkModuleSystem* module_system = GetModuleSystem(info);
  if (info.Length() < 1) {
    // TODO(cmarcelo): Throw appropriate exception or warning.
    result.SetUndefined();
    return;
  }
  v8::Handle<v8::Object> object =
      module_system->RequireNative(*v8::String::Utf8Value(info[0]));
  if (object.IsEmpty()) {
    // TODO(cmarcelo): Throw appropriate exception or warning.
    result.SetUndefined();
    return;
  }
  result.Set(object);
}

}  // namespace

XWalkModuleSystem::XWalkModuleSystem(v8::Handle<v8::Context> context) {
  v8::Isolate* isolate = context->GetIsolate();
  v8_context_.Reset(isolate, context);

  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Object> function_data = v8::Object::New(isolate);
  function_data->Set(v8::String::NewFromUtf8(isolate, kXWalkModuleSystem),
                     v8::External::New(isolate, this));
  v8::Handle<v8::FunctionTemplate> require_native_template =
      v8::FunctionTemplate::New(isolate, RequireNativeCallback, function_data);

  function_data_.Reset(isolate, function_data);
  require_native_template_.Reset(isolate, require_native_template);
}

XWalkModuleSystem::~XWalkModuleSystem() {
  DeleteExtensionModules();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  // Deleting the data will disable the functions, they'll return early. We do
  // this because it might be the case that the JS objects we created outlive
  // this object, even if we destroy the references we have.
  // TODO(cmarcelo): Add a test for this case.
  // FIXME(cmarcelo): These calls are causing crashes on shutdown with Chromium
  //                  29.0.1547.57 and had to be commented out.
  // v8::Handle<v8::Object> function_data =
  //     v8::Handle<v8::Object>::New(isolate, function_data_);
  // function_data->Delete(v8::String::New(kXWalkModuleSystem));

  require_native_template_.Reset();
  function_data_.Reset();
  v8_context_.Reset();
}

// static
XWalkModuleSystem* XWalkModuleSystem::GetModuleSystemFromContext(
    v8::Handle<v8::Context> context) {
  return reinterpret_cast<XWalkModuleSystem*>(
      context->GetAlignedPointerFromEmbedderData(
          kModuleSystemEmbedderDataIndex));
}

// static
void XWalkModuleSystem::SetModuleSystemInContext(
    std::unique_ptr<XWalkModuleSystem> module_system,
    v8::Handle<v8::Context> context) {
  context->SetAlignedPointerInEmbedderData(kModuleSystemEmbedderDataIndex,
                                           module_system.release());
}

// static
void XWalkModuleSystem::ResetModuleSystemFromContext(
    v8::Handle<v8::Context> context) {
  delete GetModuleSystemFromContext(context);
  SetModuleSystemInContext(std::unique_ptr<XWalkModuleSystem>(), context);
}

void XWalkModuleSystem::RegisterExtensionModule(
    std::unique_ptr<XWalkExtensionModule> module,
    const std::vector<std::string>& entry_points) {
  const std::string& extension_name = module->extension_name();
  if (ContainsEntryPoint(extension_name)) {
    LOG(WARNING) << "Can't register Extension Module named for extension '"
                 << extension_name << "' in the Module System because name was "
                 << "already registered.";
    return;
  }

  std::vector<std::string>::const_iterator it = entry_points.begin();
  for (; it != entry_points.end(); ++it) {
    if (ContainsEntryPoint(*it)) {
      LOG(WARNING) << "Can't register Extension Module for extension '"
                   << extension_name << "' in the Module System because "
                   << "another extension has the entry point '"
                   << *it << "'.";
      return;
    }
  }

  extension_modules_.push_back(
      ExtensionModuleEntry(extension_name, module.release(), entry_points));
}

void XWalkModuleSystem::RegisterNativeModule(
    const std::string& name, std::unique_ptr<XWalkNativeModule> module) {
  auto it = native_modules_.find(name);
  if ( it == native_modules_.end() ) {
    native_modules_[name] = std::move(module);
  } else {
    //TODO log error message
  }
}

namespace {

v8::Handle<v8::Value> EnsureTargetObjectForTrampoline(
    v8::Handle<v8::Context> context, const std::vector<std::string>& path,
    std::string* error) {
  v8::Handle<v8::Object> object = context->Global();
  v8::Isolate* isolate = context->GetIsolate();

  std::vector<std::string>::const_iterator it = path.begin();
  for (; it != path.end(); ++it) {
    v8::Handle<v8::String> part =
        v8::String::NewFromUtf8(isolate, it->c_str());
    v8::Handle<v8::Value> value = object->Get(part);

    if (value->IsUndefined()) {
      v8::Handle<v8::Object> next_object = v8::Object::New(isolate);
      object->Set(part, next_object);
      object = next_object;
      continue;
    }

    if (!value->IsObject()) {
      *error = "the property '" + *it + "' in the path is undefined";
      return v8::Undefined(isolate);
    }

    object = value.As<v8::Object>();
  }
  return object;
}

v8::Handle<v8::Value> GetObjectForPath(v8::Handle<v8::Context> context,
                                       const std::vector<std::string>& path,
                                       std::string* error) {
  v8::Handle<v8::Object> object = context->Global();
  v8::Isolate* isolate = context->GetIsolate();

  std::vector<std::string>::const_iterator it = path.begin();
  for (; it != path.end(); ++it) {
    v8::Handle<v8::String> part =
        v8::String::NewFromUtf8(isolate, it->c_str());
    v8::Handle<v8::Value> value = object->Get(part);

    if (!value->IsObject()) {
      *error = "the property '" + *it + "' in the path is undefined";
      return v8::Undefined(isolate);
    }

    object = value.As<v8::Object>();
  }

  return object;
}

}  // namespace

bool XWalkModuleSystem::SetTrampolineAccessorForEntryPoint(
    v8::Handle<v8::Context> context,
    const std::string& entry_point,
    v8::Local<v8::External> user_data) {
  std::vector<std::string> path = base::SplitString(
      entry_point, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::string basename = path.back();
  path.pop_back();

  std::string error;
  v8::Handle<v8::Value> value =
      EnsureTargetObjectForTrampoline(context, path, &error);
  if (value->IsUndefined()) {
    LOG(WARNING) << "Error installing trampoline for " << entry_point
                 << ": " << error << ".";
    return false;
  }

  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Array> params = v8::Array::New(isolate);
  v8::Local<v8::String> entry = v8::String::NewFromUtf8(isolate,
                                                        entry_point.c_str());
  params->Set(v8::Integer::New(isolate, 0), user_data);
  params->Set(v8::Integer::New(isolate, 1), entry);

  // FIXME(cmarcelo): ensure that trampoline is readonly.
  value.As<v8::Object>()->SetAccessor(context,
      v8::String::NewFromUtf8(isolate, basename.c_str()),
      TrampolineCallback, TrampolineSetterCallback, params).ToChecked();
  return true;
}

// static
bool XWalkModuleSystem::DeleteAccessorForEntryPoint(
    v8::Handle<v8::Context> context,
    const std::string& entry_point) {
  std::vector<std::string> path = base::SplitString(
      entry_point, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::string basename = path.back();
  path.pop_back();

  std::string error;
  v8::Handle<v8::Value> value = GetObjectForPath(context, path, &error);
  if (value->IsUndefined()) {
    LOG(WARNING) << "Error retrieving object for " << entry_point
                 << ": " << error << ".";
    return false;
  }

  value.As<v8::Object>()->Delete(
      v8::String::NewFromUtf8(context->GetIsolate(), basename.c_str()));
  return true;
}

bool XWalkModuleSystem::InstallTrampoline(v8::Handle<v8::Context> context,
                                          ExtensionModuleEntry* entry) {
  v8::Local<v8::External> entry_ptr = v8::External::New(context->GetIsolate(),
                                                        entry);
  bool ret;

  ret = SetTrampolineAccessorForEntryPoint(context, entry->name, entry_ptr);
  if (!ret) {
    LOG(WARNING) << "Error installing trampoline for '"
                 << entry->name << "'.";
    return false;
  }

  std::vector<std::string>::const_iterator it = entry->entry_points.begin();
  for (; it != entry->entry_points.end(); ++it) {
    ret = SetTrampolineAccessorForEntryPoint(context, *it, entry_ptr);
    if (!ret) {
      // TODO(vcgomes): Remove already added trampolines when it fails.
      LOG(WARNING) << "Error installing trampoline for '"
                   << entry->name << "'.";
      return false;
    }
  }

  return true;
}

v8::Handle<v8::Object> XWalkModuleSystem::RequireNative(
    const std::string& name) {
  NativeModuleMap::iterator it = native_modules_.find(name);
  if (it == native_modules_.end())
    return v8::Handle<v8::Object>();
  return it->second->NewInstance();
}

void XWalkModuleSystem::Initialize() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> context = GetV8Context();
  v8::Handle<v8::FunctionTemplate> require_native_template =
      v8::Local<v8::FunctionTemplate>::New(isolate, require_native_template_);
  v8::Handle<v8::Function> require_native =
      require_native_template->GetFunction();

  MarkModulesWithTrampoline();

  ExtensionModules::iterator it = extension_modules_.begin();
  for (; it != extension_modules_.end(); ++it) {
    if (it->use_trampoline && InstallTrampoline(context, &*it))
      continue;
    it->module->LoadExtensionCode(context, require_native);
    EnsureExtensionNamespaceIsReadOnly(context, it->name);
  }
}

v8::Handle<v8::Context> XWalkModuleSystem::GetV8Context() {
  return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), v8_context_);
}

bool XWalkModuleSystem::ContainsEntryPoint(
    const std::string& entry) {
  ExtensionModules::iterator it = extension_modules_.begin();
  for (; it != extension_modules_.end(); ++it) {
    if (it->name == entry)
      return true;

    std::vector<std::string>::iterator entry_it = std::find(
        it->entry_points.begin(), it->entry_points.end(), entry);
    if (entry_it != it->entry_points.end()) {
      return true;
    }
  }
  return false;
}

void XWalkModuleSystem::DeleteExtensionModules() {
  for (ExtensionModules::iterator it = extension_modules_.begin();
       it != extension_modules_.end(); ++it) {
    delete it->module;
  }
  extension_modules_.clear();
}

// static
void XWalkModuleSystem::LoadExtensionForTrampoline(
    v8::Isolate* isolate,
    v8::Local<v8::Value> data) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Array> params = data.As<v8::Array>();
  void* ptr = params->Get(
      v8::Integer::New(isolate, 0)).As<v8::External>()->Value();

  ExtensionModuleEntry* entry = static_cast<ExtensionModuleEntry*>(ptr);

  if (!entry)
    return;
  v8::Handle<v8::Context> context = params->CreationContext();

  // Switch to the target context in case cross-context reference.
  v8::Context::Scope context_scope(context);

  DeleteAccessorForEntryPoint(context, entry->name);

  std::vector<std::string>::const_iterator it = entry->entry_points.begin();
  for (; it != entry->entry_points.end(); ++it) {
    DeleteAccessorForEntryPoint(context, *it);
  }

  XWalkModuleSystem* module_system = GetModuleSystemFromContext(context);
  v8::Handle<v8::FunctionTemplate> require_native_template =
        v8::Local<v8::FunctionTemplate>::New(
            isolate,
            module_system->require_native_template_);

  XWalkExtensionModule* module = entry->module;
  module->LoadExtensionCode(context,
                            require_native_template->GetFunction());

  module_system->EnsureExtensionNamespaceIsReadOnly(context, entry->name);
}

// static
v8::Handle<v8::Value> XWalkModuleSystem::RefetchHolder(
    v8::Isolate* isolate,
    v8::Local<v8::Value> data) {
  v8::Local<v8::Array> params = data.As<v8::Array>();
  const std::string entry_point = *v8::String::Utf8Value(
      params->Get(v8::Integer::New(isolate, 1)).As<v8::String>());

  std::vector<std::string> path = base::SplitString(
      entry_point, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  path.pop_back();

  std::string error;
  return GetObjectForPath(params->CreationContext(), path, &error);
}

// static
void XWalkModuleSystem::TrampolineCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  XWalkModuleSystem::LoadExtensionForTrampoline(info.GetIsolate(), info.Data());
  v8::Handle<v8::Value> holder = RefetchHolder(info.GetIsolate(), info.Data());
  if (holder->IsUndefined())
    return;

  info.GetReturnValue().Set(holder.As<v8::Object>()->Get(property));
}

// static
void XWalkModuleSystem::TrampolineSetterCallback(
    v8::Local<v8::Name> property,
    v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  XWalkModuleSystem::LoadExtensionForTrampoline(info.GetIsolate(), info.Data());
  v8::Handle<v8::Value> holder = RefetchHolder(info.GetIsolate(), info.Data());
  if (holder->IsUndefined())
    return;

  holder.As<v8::Object>()->Set(property, value);
}

XWalkModuleSystem::ExtensionModuleEntry::ExtensionModuleEntry(
  const std::string& name,
  XWalkExtensionModule* module,
  const std::vector<std::string>& entry_points) :
    name(name), module(module), use_trampoline(true),
    entry_points(entry_points) {
}

XWalkModuleSystem::ExtensionModuleEntry::ExtensionModuleEntry(
    const ExtensionModuleEntry& other) = default;

XWalkModuleSystem::ExtensionModuleEntry::~ExtensionModuleEntry() {
}

// Returns whether the name of first is prefix of the second, considering "."
// character as a separator. So "a" is prefix of "a.b" but not of "ab".
bool XWalkModuleSystem::ExtensionModuleEntry::IsPrefix(
    const ExtensionModuleEntry& first,
    const ExtensionModuleEntry& second) {
  const std::string& p = first.name;
  const std::string& s = second.name;
  return s.size() > p.size() && s[p.size()] == '.'
      && std::mismatch(p.begin(), p.end(), s.begin()).first == p.end();
}

// Mark the extension modules that we want to setup "trampolines"
// instead of loading the code directly. The current algorithm is very
// simple: we only create trampolines for extensions that are leaves
// in the namespace tree.
//
// For example, if there are two extensions "echo" and "echo.time",
// the first one won't be marked with trampoline, but the second one
// will. So we'll only load code for "echo" extension.
void XWalkModuleSystem::MarkModulesWithTrampoline() {
  std::sort(extension_modules_.begin(), extension_modules_.end());

  ExtensionModules::iterator it = extension_modules_.begin();
  while (it != extension_modules_.end()) {
    it = std::adjacent_find(it, extension_modules_.end(),
                            &ExtensionModuleEntry::IsPrefix);
    if (it == extension_modules_.end())
      break;
    it->use_trampoline = false;
    ++it;
  }
}

void XWalkModuleSystem::EnsureExtensionNamespaceIsReadOnly(
    v8::Handle<v8::Context> context,
    const std::string& extension_name) {
  std::vector<std::string> path = base::SplitString(
      extension_name, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::string basename = path.back();
  path.pop_back();

  std::string error;
  v8::Handle<v8::Value> value = GetObjectForPath(context, path, &error);
  if (value->IsUndefined()) {
    LOG(WARNING) << "Error retrieving object for " << extension_name
                 << ": " << error << ".";
    return;
  }

  v8::Handle<v8::String> v8_extension_name(
      v8::String::NewFromUtf8(context->GetIsolate(), basename.c_str()));
  value.As<v8::Object>()->DefineOwnProperty(
      context,
      v8_extension_name, value.As<v8::Object>()->Get(v8_extension_name),
      v8::ReadOnly).FromJust();
}

}  // namespace extensions
}  // namespace xwalk
