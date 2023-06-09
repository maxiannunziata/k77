// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_RUNTIME_RESOURCE_DISPATCHER_HOST_DELEGATE_ANDROID_H_
#define XWALK_RUNTIME_BROWSER_RUNTIME_RESOURCE_DISPATCHER_HOST_DELEGATE_ANDROID_H_

#include <map>
#include <utility>

#include "content/public/browser/content_browser_client.h"
#include "xwalk/runtime/browser/runtime_resource_dispatcher_host_delegate.h"

namespace content {
class ResourceDispatcherHostLoginDelegate;
struct ResourceResponse;
}  // namespace content

namespace IPC {
class Sender;
}  // namespace IPC

namespace xwalk {

class IoThreadClientThrottle;

class RuntimeResourceDispatcherHostDelegateAndroid
    : public RuntimeResourceDispatcherHostDelegate {
 public:
  RuntimeResourceDispatcherHostDelegateAndroid();
  ~RuntimeResourceDispatcherHostDelegateAndroid() override;

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
  void OnResponseStarted(net::URLRequest* request, content::ResourceContext* resource_context,
                         network::ResourceResponse* response) override;
  void RemovePendingThrottleOnIoThread(IoThreadClientThrottle* throttle);

  static void OnIoThreadClientReady(int new_render_process_id,
                                    int new_render_frame_id);
  static void AddPendingThrottle(int render_process_id,
                                 int render_frame_id,
                                 IoThreadClientThrottle* pending_throttle);

 private:
  // These methods must be called on IO thread.
  void OnIoThreadClientReadyInternal(int new_render_process_id,
                                     int new_render_frame_id);
  void AddPendingThrottleOnIoThread(int render_process_id,
                                    int render_frame_id,
                                    IoThreadClientThrottle* pending_throttle);

  typedef std::pair<int, int> FrameRouteIDPair;
  typedef std::map<FrameRouteIDPair, IoThreadClientThrottle*>
      PendingThrottleMap;

  // Only accessed on the IO thread.
  PendingThrottleMap pending_throttles_;

  DISALLOW_COPY_AND_ASSIGN(RuntimeResourceDispatcherHostDelegateAndroid);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_RUNTIME_RESOURCE_DISPATCHER_HOST_DELEGATE_ANDROID_H_
