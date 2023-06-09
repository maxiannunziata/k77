// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/application/renderer/application_native_module.h"

#include "base/logging.h"
#include "content/public/renderer/v8_value_converter.h"
#include "content/public/renderer/render_view.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"

namespace xwalk {
namespace application {

namespace {

// This is the key used in the data object passed to our callbacks to store a
// pointer back to ApplicationNativeModule.
const char* kApplicationNativeModule = "kApplicationNativeModule";

}  // namespace

void ApplicationNativeModule::GetViewByIDCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().SetUndefined();
  if (info.Length() != 1 || !info[0]->IsInt32())
    return;

  int main_routing_id = info[0]->ToInt32(info.GetIsolate()->GetCurrentContext()).ToLocalChecked()->Value();
  content::RenderView* render_view =
    content::RenderView::FromRoutingID(main_routing_id);
  if (!render_view)
    return;

  blink::WebView* webview = render_view->GetWebView();
  v8::Handle<v8::Context> context =
    webview->MainFrame()->ToWebLocalFrame()->MainWorldScriptContext();
  v8::Handle<v8::Value> window = context->Global();
  info.GetReturnValue().Set(window);
}

void ApplicationNativeModule::GetCurrentRoutingIDCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  blink::WebLocalFrame* webframe =
    blink::WebLocalFrame::FrameForCurrentContext();
  DCHECK(webframe);

  blink::WebView* webview = webframe->View();
  DCHECK(webview);

  content::RenderView* render_view = content::RenderView::FromWebView(webview);
  info.GetReturnValue().Set(render_view->GetRoutingID());
}

ApplicationNativeModule::ApplicationNativeModule() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Object> function_data = v8::Object::New(isolate);
  function_data->Set(context,
      v8::String::NewFromUtf8(isolate, kApplicationNativeModule).ToLocalChecked(),
      v8::External::New(isolate, this)).Check();

  // Register native function templates to object template here.
  v8::Handle<v8::ObjectTemplate> object_template =
      v8::ObjectTemplate::New(isolate);
  object_template->Set(
      isolate,
      "getViewByID",
      v8::FunctionTemplate::New(isolate,
                                &ApplicationNativeModule::GetViewByIDCallback,
                                function_data));
  object_template->Set(
      isolate,
      "getCurrentRoutingID",
      v8::FunctionTemplate::New(
          isolate,
          &ApplicationNativeModule::GetCurrentRoutingIDCallback,
          function_data));

  function_data_.Reset(isolate, function_data);
  object_template_.Reset(isolate, object_template);
}

ApplicationNativeModule::~ApplicationNativeModule() {
  object_template_.Reset();
  function_data_.Reset();
}

v8::Handle<v8::Object> ApplicationNativeModule::NewInstance() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Handle<v8::ObjectTemplate> object_template =
      v8::Local<v8::ObjectTemplate>::New(isolate, object_template_);
  return handle_scope.Escape(object_template->NewInstance(context).ToLocalChecked());
}

}  // namespace application
}  // namespace xwalk
