// Copyright 2016 The Intel Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/xwalk_form_database.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/android/core_refactor/xwalk_refactor_native_jni/XWalkFormDatabase_jni.h"

namespace xwalk {

namespace {

XWalkFormDatabaseService* GetFormDatabaseService() {

  XWalkBrowserContext* context = XWalkBrowserContext::GetDefault();
  XWalkFormDatabaseService* service = context->GetFormDatabaseService();
  return service;
}

} // anonymous namespace

// static
jboolean JNI_XWalkFormDatabase_HasFormData(JNIEnv*) {
  return GetFormDatabaseService()->HasFormData();
}

// static
void JNI_XWalkFormDatabase_ClearFormData(JNIEnv*) {
  GetFormDatabaseService()->ClearFormData();
}

bool RegisterXWalkFormDatabase(JNIEnv* env) {
  return false;
}

} // namespace xwalk
