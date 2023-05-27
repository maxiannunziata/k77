// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_COMMON_XWALK_LOCALIZED_ERROR_H_
#define XWALK_RUNTIME_COMMON_XWALK_LOCALIZED_ERROR_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace blink {
struct WebURLError;
}

namespace xwalk {
class LocalizedError {
 public:
  // Returns a description of the encountered error.
  static base::string16 GetErrorDetails(const std::string& error_domain, const blink::WebURLError& error, bool is_post);

  static const char kHttpErrorDomain[];
  static const char kDnsProbeErrorDomain[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LocalizedError);
};

} // namesapce xwalk
#endif  // XWALK_RUNTIME_COMMON_XWALK_LOCALIZED_ERROR_H_
