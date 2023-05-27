// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_COOKIE_MANAGER_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_COOKIE_MANAGER_H_

#include <jni.h>
#include "base/memory/scoped_refptr.h"

namespace base {
class SingleThreadTaskRunner;
}
namespace net {
class CookieStore;
}  // namespace net

namespace xwalk {

scoped_refptr<base::SingleThreadTaskRunner> GetCookieStoreTaskRunner();
net::CookieStore* GetCookieStore();

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_COOKIE_MANAGER_H_
