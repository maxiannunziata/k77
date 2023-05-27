// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/extensions/renderer/xwalk_v8_utils.h"

#include "base/strings/stringprintf.h"
#include "v8/include/v8.h"

namespace xwalk {
namespace extensions {

// iotto: see chrome/./test/base/v8_unit_test.cc:316
std::string ExceptionToString(const v8::TryCatch& try_catch) {
  std::string str;
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope handle_scope(isolate);
  v8::String::Utf8Value exception(isolate,try_catch.Exception());
  v8::Local<v8::Message> message(try_catch.Message());
  if (message.IsEmpty()) {
    str.append(base::StringPrintf("%s\n", *exception));
  } else {

    v8::String::Utf8Value filename(isolate,message->GetScriptResourceName());
    int linenum = message->GetLineNumber(context).ToChecked();
    int colnum = message->GetStartColumn();
    str.append(base::StringPrintf(
        "%s:%i:%i %s\n", *filename, linenum, colnum, *exception));
    v8::String::Utf8Value sourceline(isolate,message->GetSourceLine(context).ToLocalChecked());
    str.append(base::StringPrintf("%s\n", *sourceline));
  }
  return str;
}

}  // namespace extensions
}  // namespace xwalk
