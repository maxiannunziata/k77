// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/runtime_quota_permission_context.h"

namespace xwalk {

RuntimeQuotaPermissionContext::RuntimeQuotaPermissionContext() {}

void RuntimeQuotaPermissionContext::RequestQuotaPermission(
      const content::StorageQuotaParams& params,
      int render_process_id,
      const PermissionCallback& callback) {
  LOG(WARNING) << "iotto " << __func__ << " CHECK!";
  // TODO(wang16): Handle request according to app's manifest declaration.
  callback.Run(content::QuotaPermissionContext::QUOTA_PERMISSION_RESPONSE_ALLOW);
}

RuntimeQuotaPermissionContext::~RuntimeQuotaPermissionContext() {}

}  // namespace xwalk
