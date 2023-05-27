/*
 * xwalk_cookie_change_dispatcher_wrapper.h
 *
 *  Created on: Jan 14, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_NET_XWALK_COOKIE_CHANGE_DISPATCHER_WRAPPER_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_NET_XWALK_COOKIE_CHANGE_DISPATCHER_WRAPPER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "net/cookies/cookie_change_dispatcher.h"

class GURL;

namespace xwalk {

class XWalkCookieChangeDispatcherWrapper : public net::CookieChangeDispatcher {
 public:
  XWalkCookieChangeDispatcherWrapper();
  ~XWalkCookieChangeDispatcherWrapper() override;

  // net::CookieChangeDispatcher
  std::unique_ptr<net::CookieChangeSubscription> AddCallbackForCookie(const GURL& url, const std::string& name,
                                                                      net::CookieChangeCallback callback)
                                                                          override WARN_UNUSED_RESULT;
  std::unique_ptr<net::CookieChangeSubscription> AddCallbackForUrl(const GURL& url, net::CookieChangeCallback callback)
      override WARN_UNUSED_RESULT;
  std::unique_ptr<net::CookieChangeSubscription> AddCallbackForAllChanges(net::CookieChangeCallback callback)
      override WARN_UNUSED_RESULT;

 private:
  DISALLOW_COPY_AND_ASSIGN(XWalkCookieChangeDispatcherWrapper);
};

}  // namespace xwalk

#endif /* XWALK_RUNTIME_BROWSER_ANDROID_NET_XWALK_COOKIE_CHANGE_DISPATCHER_WRAPPER_H_ */
