// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/net_disk_cache_remover.h"

#include "base/bind_helpers.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/http/http_transaction_factory.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_context.h"

using content::BrowserThread;
using disk_cache::Backend;
using net::URLRequestContextGetter;

namespace {
// Everything is called and accessed on the IO thread.

void Noop(int rv) {
  DCHECK(rv == net::OK);
}

void CallDoomAllEntries(Backend** backend, int rv) {
  DCHECK(rv == net::OK);
  (*backend)->DoomAllEntries(base::BindOnce(&Noop));
}

void CallDoomEntry(Backend** backend, const std::string& key, net::RequestPriority priority, int rv) {
  DCHECK(rv == net::OK);
  (*backend)->DoomEntry(key, priority, base::BindOnce(&Noop));
}

void ClearHttpDiskCacheOfContext(URLRequestContextGetter* context_getter,
                                 const std::string& key) {
  typedef Backend* BackendPtr;  // Make line below easier to understand.
  BackendPtr* backend_ptr = new BackendPtr(NULL);
  net::CompletionOnceCallback callback;
  if (!key.empty()) {
    callback = base::BindOnce(&CallDoomEntry,
                          base::Owned(backend_ptr),
                          key, net::RequestPriority::DEFAULT_PRIORITY);
  } else {
    callback = base::BindOnce(&CallDoomAllEntries,
                          base::Owned(backend_ptr));
  }

  context_getter->GetURLRequestContext()->
    http_transaction_factory()->GetCache()->GetBackend(backend_ptr, std::move(callback));
}

void ClearHttpDiskCacheOnIoThread(
    URLRequestContextGetter* main_context_getter,
    URLRequestContextGetter* media_context_getter,
    const std::string& key) {
  ClearHttpDiskCacheOfContext(main_context_getter, key);
  ClearHttpDiskCacheOfContext(media_context_getter, key);
}

}  // namespace

namespace xwalk {

void RemoveHttpDiskCache(content::RenderProcessHost* render_process_host,
                         const std::string& key) {
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&ClearHttpDiskCacheOnIoThread,
                 base::Unretained(render_process_host->GetStoragePartition()->
                     GetURLRequestContext()),
                 base::Unretained(render_process_host->GetStoragePartition()->
                     GetMediaURLRequestContext()),
                 key));
}

}  // namespace xwalk
