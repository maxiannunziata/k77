// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "xwalk/runtime/renderer/isolated_file_system.h"

#include "base/logging.h"
#include "content/public/renderer/v8_value_converter.h"
#include "content/public/renderer/render_view.h"
#include "storage/common/fileapi/file_system_types.h"
#include "storage/common/fileapi/file_system_util.h"
#include "third_party/blink/public/platform/web_file_system_type.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document_loader.h"
#include "third_party/blink/public/web/web_dom_file_system.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"
#include "v8/include/v8.h"
#include "xwalk/extensions/renderer/xwalk_module_system.h"

using content::RenderView;
using blink::WebFrame;
using blink::WebView;

namespace {

// This is the key used in the data object passed to our callbacks to store a
// pointer back to IsolatedFileSystem.
const char* kIsolatedFileSystemModule = "kIsolatedFileSystemModule";

}  // namespace

namespace xwalk {
namespace extensions {

void IsolatedFileSystem::GetIsolatedFileSystem(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  CHECK(info.Length() == 1 || info.Length() == 2);
  CHECK(info[0]->IsString());

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  blink::WebLocalFrame* webframe =
      blink::WebLocalFrame::FrameForCurrentContext();
  CHECK(webframe);
  std::string file_system_id(*v8::String::Utf8Value(isolate,info[0]));

  blink::WebDocumentLoader * loader = webframe->GetDocumentLoader();

//  blink::WebDataSource* data_source = webframe->ProvisionalDataSource() ?
//      webframe->ProvisionalDataSource() : webframe->DataSource();
  CHECK(loader);
  GURL context_url(loader->GetUrl());

  // In instrument test, context_url.GetOrigin() returns emtpy string.
  // That causes app crash. So assign "file:///" as default value to
  // origin to avoid this.
  GURL origin("file:///");
  if (!context_url.SchemeIs(url::kDataScheme) &&
      !context_url.GetOrigin().is_empty()) {
    origin = context_url.GetOrigin();
  }
  std::string name(storage::GetIsolatedFileSystemName(origin, file_system_id));

  // The optional second argument is the subfolder within the isolated file
  // system at which to root the DOMFileSystem we're returning to the caller.
  std::string optional_root_name = "";

  blink::WebURL root(GURL(storage::GetIsolatedFileSystemRootURIString(
      origin,
      file_system_id,
      optional_root_name)));
  info.GetReturnValue().Set(blink::WebDOMFileSystem::Create(webframe,
      blink::kWebFileSystemTypeIsolated,
      blink::WebString::FromUTF8(name),
      root).ToV8Value(isolate->GetCurrentContext()->Global(), isolate));
}

IsolatedFileSystem::IsolatedFileSystem() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Object> function_data = v8::Object::New(isolate);
  function_data->Set(context,
      v8::String::NewFromUtf8(isolate, kIsolatedFileSystemModule).ToLocalChecked(),
      v8::External::New(isolate, this)).Check();

  // Register native function templates to object template here.
  v8::Handle<v8::ObjectTemplate> object_template =
      v8::ObjectTemplate::New(isolate);
  object_template->Set(
      isolate,
      "getIsolatedFileSystem",
      v8::FunctionTemplate::New(isolate,
                                &IsolatedFileSystem::GetIsolatedFileSystem,
                                function_data));

  function_data_.Reset(isolate, function_data);
  object_template_.Reset(isolate, object_template);
}

IsolatedFileSystem::~IsolatedFileSystem() {
  object_template_.Reset();
  function_data_.Reset();
}

v8::Handle<v8::Object> IsolatedFileSystem::NewInstance() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Handle<v8::ObjectTemplate> object_template =
      v8::Local<v8::ObjectTemplate>::New(isolate, object_template_);
  return handle_scope.Escape(object_template->NewInstance(context).ToLocalChecked());
}

}  // namespace extensions
}  // namespace xwalk
