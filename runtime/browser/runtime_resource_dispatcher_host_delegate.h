// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_RUNTIME_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define XWALK_RUNTIME_BROWSER_RUNTIME_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include "content/public/browser/resource_dispatcher_host_delegate.h"
//#include "base/memory/scoped_vector.h"

namespace xwalk {

class RuntimeResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  RuntimeResourceDispatcherHostDelegate();
  ~RuntimeResourceDispatcherHostDelegate() override;

  static void ResourceDispatcherHostCreated();
  static std::unique_ptr<RuntimeResourceDispatcherHostDelegate> Create();

  void RequestBeginning(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::AppCacheService* appcache_service,
      content::ResourceType resource_type,
      std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) override;

  void DownloadStarting(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      bool is_content_initiated,
      bool must_download,
      bool is_new_request,
      std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) override;
//  bool HandleExternalProtocol(
//      const GURL& url,
//      content::ResourceRequestInfo* info) override;
 private:
  DISALLOW_COPY_AND_ASSIGN(RuntimeResourceDispatcherHostDelegate);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_RUNTIME_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
