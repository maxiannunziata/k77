// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_SCOPED_ALLOW_WAIT_FOR_LEGACY_WEB_VIEW_API_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_SCOPED_ALLOW_WAIT_FOR_LEGACY_WEB_VIEW_API_H_

#include "base/threading/thread_restrictions.h"

// This class is only available when building the chromium back-end for android
// webview: it is required where API backward compatibility demands that the UI
// thread must block waiting on other threads e.g. to obtain a synchronous
// return value. Long term, asynchronous overloads of all such methods will be
// added in the public API, and and no new uses of this will be allowed.
class ScopedAllowWaitForLegacyWebViewApi {
 private:
  base::ScopedAllowBaseSyncPrimitives wait;
};

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_SCOPED_ALLOW_WAIT_FOR_LEGACY_WEB_VIEW_API_H_
