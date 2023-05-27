// Copyright (c) 2014 Intel Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/xwalk_view_delegate.h"

#include "base/android/jni_android.h"
#include "build/build_config.h"
#include "xwalk/runtime/android/core_refactor/xwalk_refactor_native_jni/XWalkViewDelegate_jni.h"

namespace xwalk {

jboolean JNI_XWalkViewDelegate_IsLibraryBuiltForIA(JNIEnv* env) {
#if defined(ARCH_CPU_X86) || defined(ARCH_CPU_X86_64)
  return JNI_TRUE;
#else
  return JNI_FALSE;
#endif
}

bool RegisterXWalkViewDelegate(JNIEnv* env) {
  return false;
}

}  // namespace xwalk
