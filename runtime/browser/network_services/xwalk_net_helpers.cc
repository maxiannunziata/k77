/*
 * xwalk_net_helpers.cc
 *
 *  Created on: Feb 16, 2020
 *      Author: iotto
 */

#include "xwalk/runtime/browser/network_services/xwalk_net_helpers.h"

#include "net/base/load_flags.h"
#include "url/gurl.h"
#include "xwalk/runtime/browser/android/xwalk_contents_io_thread_client.h"
#include "xwalk/runtime/browser/android/net/url_constants.h"

namespace xwalk {

namespace {
int UpdateCacheControlFlags(int load_flags, int cache_control_flags) {
  const int all_cache_control_flags = net::LOAD_BYPASS_CACHE | net::LOAD_VALIDATE_CACHE
      | net::LOAD_SKIP_CACHE_VALIDATION | net::LOAD_ONLY_FROM_CACHE;
  DCHECK_EQ((cache_control_flags & all_cache_control_flags), cache_control_flags);
  load_flags &= ~all_cache_control_flags;
  load_flags |= cache_control_flags;
  return load_flags;
}

// Gets the net-layer load_flags which reflect |client|'s cache mode.
int GetCacheModeForClient(XWalkContentsIoThreadClient* client) {
  DCHECK(client);
  XWalkContentsIoThreadClient::CacheMode cache_mode = client->GetCacheMode();
  switch (cache_mode) {
    case XWalkContentsIoThreadClient::LOAD_CACHE_ELSE_NETWORK:
      // If the resource is in the cache (even if expired), load from cache.
      // Otherwise, fall back to network.
      return net::LOAD_SKIP_CACHE_VALIDATION;
    case XWalkContentsIoThreadClient::LOAD_NO_CACHE:
      // Always load from the network, don't use the cache.
      return net::LOAD_BYPASS_CACHE;
    case XWalkContentsIoThreadClient::LOAD_CACHE_ONLY:
      // If the resource is in the cache (even if expired), load from cache. Do
      // not fall back to the network.
      return net::LOAD_ONLY_FROM_CACHE | net::LOAD_SKIP_CACHE_VALIDATION;
    default:
      // If the resource is in the cache (and is valid), load from cache.
      // Otherwise, fall back to network. This is the usual (default) case.
      return 0;
  }
}

}  // namespace

int UpdateLoadFlags(int load_flags, XWalkContentsIoThreadClient* client) {
  if (!client)
    return load_flags;

  if (client->ShouldBlockNetworkLoads()) {
    return UpdateCacheControlFlags(
        load_flags,
        net::LOAD_ONLY_FROM_CACHE | net::LOAD_SKIP_CACHE_VALIDATION);
  }

  int cache_mode = GetCacheModeForClient(client);
  if (!cache_mode)
    return load_flags;

  return UpdateCacheControlFlags(load_flags, cache_mode);
}

bool ShouldBlockURL(const GURL& url, XWalkContentsIoThreadClient* client) {
  if (!client)
    return false;

  // Part of implementation of WebSettings.allowContentAccess.
  if (url.SchemeIs(url::kContentScheme) && client->ShouldBlockContentUrls())
    return true;

  // Part of implementation of WebSettings.allowFileAccess.
  if (url.SchemeIsFile() && client->ShouldBlockFileUrls()) {
    // Application's assets and resources are always available.
    return !IsAndroidSpecialFileUrl(url);
  }

  return client->ShouldBlockNetworkLoads() && url.SchemeIs(url::kFtpScheme);
}

} // namespace xwalk


