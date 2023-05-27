// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/xwalk_path_helper.h"

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/bind.h"
#include "xwalk/extensions/common/xwalk_extension.h"
#include "xwalk/runtime/browser/xwalk_browser_main_parts.h"
#include "xwalk/runtime/browser/xwalk_content_browser_client.h"

#include "xwalk/runtime/android/core_refactor/xwalk_refactor_native_jni/XWalkPathHelper_jni.h"
using base::android::JavaParamRef;

namespace xwalk {

typedef std::map<std::string, base::FilePath> VirtualRootMap;
VirtualRootMap XWalkPathHelper::virtual_root_map_;

VirtualRootMap XWalkPathHelper::GetVirtualRootMap() {
  return virtual_root_map_;
}

void XWalkPathHelper::SetDirectory(const std::string& virtualRoot,
                                   const std::string& path) {
  virtual_root_map_[virtualRoot] =
      base::FilePath::FromUTF8Unsafe(path);
}

static void JNI_XWalkPathHelper_SetDirectory(JNIEnv* env,
                                             const JavaParamRef<jstring>& virtualRoot,
                                             const JavaParamRef<jstring>& path) {
  const char* strVirtualRoot = env->GetStringUTFChars(virtualRoot, NULL);
  const char* strPath = env->GetStringUTFChars(path, NULL);
  XWalkPathHelper::SetDirectory(std::string(strVirtualRoot), std::string(strPath));
  env->ReleaseStringUTFChars(virtualRoot, strVirtualRoot);
  env->ReleaseStringUTFChars(path, strPath);
}

bool RegisterXWalkPathHelper(JNIEnv* env) {
//  return RegisterNativesImpl(env);
  return false;
}

}  // namespace xwalk
