// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_CONTENTS_IO_THREAD_CLIENT_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_CONTENTS_IO_THREAD_CLIENT_H_

#include <memory>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback_forward.h"
#include "base/task/post_task.h"

class GURL;

namespace content {
class WebContents;
}

namespace net {
class HttpResponseHeaders;
class URLRequest;
}

namespace xwalk {

class XWalkWebResourceResponse;
struct XWalkWebResourceRequest;

// This class provides a means of calling Java methods on an instance that has
// a 1:1 relationship with a WebContents instance directly from the IO thread.
//
// Specifically this is used to associate URLRequests with the WebContents that
// the URLRequest is made for.
//
// The native class is intended to be a short-lived handle that pins the
// Java-side instance. It is preferable to use the static getter methods to
// obtain a new instance of the class rather than holding on to one for
// prolonged periods of time (see note for more details).
//
// Note: The native XWalkContentsIoThreadClient instance has a Global ref to
// the Java object. By keeping the native XWalkContentsIoThreadClient
// instance alive you're also prolonging the lifetime of the Java instance, so
// don't keep a XWalkContentsIoThreadClient if you don't need to.
class XWalkContentsIoThreadClient {
 public:
  // Corresponds to WebSettings cache mode constants.
  enum CacheMode {
    LOAD_DEFAULT = -1,
    LOAD_NORMAL = 0,
    LOAD_CACHE_ELSE_NETWORK = 1,
    LOAD_NO_CACHE = 2,
    LOAD_CACHE_ONLY = 3,
  };

  // Either |pending_associate| is true or |jclient| holds a non-null
  // Java object.
  XWalkContentsIoThreadClient(bool pending_associate, const base::android::JavaRef<jobject>& jclient);
  ~XWalkContentsIoThreadClient();

  // Called when XWalkContent is created before there is a Java client.
  static void RegisterPendingContents(content::WebContents* web_contents);

  // Associates the |jclient| instance (which must implement the
  // XWalkContentsIoThreadClient Java interface) with the |web_contents|.
  // This should be called at most once per |web_contents|.
  static void Associate(content::WebContents* web_contents, const base::android::JavaRef<jobject>& jclient);

  // This will attempt to fetch the XWalkContentsIoThreadClient for the given
  // |render_process_id|, |render_frame_id| pair.
  // This method can be called from any thread.
  // An empty scoped_ptr is a valid return value.
  static std::unique_ptr<XWalkContentsIoThreadClient> FromID(int render_process_id, int render_frame_id);

  // This map is useful when browser side navigations are enabled as
  // render_frame_ids will not be valid anymore for some of the navigations.
  static std::unique_ptr<XWalkContentsIoThreadClient> FromID(int frame_tree_node_id);

  // Called on the IO thread when a subframe is created.
  static void SubFrameCreated(int render_process_id, int parent_render_frame_id, int child_render_frame_id);

  // Returns whether this is a new pop up that is still waiting for association
  // with the java counter part.
  bool PendingAssociation() const;

  // Retrieve CacheMode setting value of this XWalkContent.
  // This method is called on the IO thread only.
  CacheMode GetCacheMode() const;

  // This method is called on the IO thread only.
  std::unique_ptr<XWalkWebResourceResponse> ShouldInterceptRequest(const GURL& location,
                                                                   const net::URLRequest* request);

  // This method is called on the IO thread only.
  using ShouldInterceptRequestResultCallback = base::OnceCallback<void(std::unique_ptr<XWalkWebResourceResponse>)>;
  void ShouldInterceptRequestAsync(XWalkWebResourceRequest request, ShouldInterceptRequestResultCallback callback);

  // Retrieve the AllowContentAccess setting value of this XWalkContent.
  // This method is called on the IO thread only.
  bool ShouldBlockContentUrls() const;

  // Retrieve the AllowFileAccess setting value of this XWalkContent.
  // This method is called on the IO thread only.
  bool ShouldBlockFileUrls() const;

  // Retrieve the BlockNetworkLoads setting value of this XWalkContent.
  // This method is called on the IO thread only.
  bool ShouldBlockNetworkLoads() const;

  // Retrieve the AcceptThirdPartyCookies setting value of this AwContents.
  bool ShouldAcceptThirdPartyCookies() const;

  void OnReceivedResponseHeaders(const net::URLRequest* request, const net::HttpResponseHeaders* response_headers);

 private:
  bool pending_association_;
  base::android::ScopedJavaGlobalRef<jobject> java_object_;
  base::android::ScopedJavaGlobalRef<jobject> bg_thread_client_object_;
  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner_ = base::CreateSequencedTaskRunnerWithTraits( {
      base::MayBlock() });

  DISALLOW_COPY_AND_ASSIGN(XWalkContentsIoThreadClient);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_CONTENTS_IO_THREAD_CLIENT_H_
