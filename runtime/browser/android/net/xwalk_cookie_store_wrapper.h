// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_NET_XWALK_COOKIE_STORE_WRAPPER_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_NET_XWALK_COOKIE_STORE_WRAPPER_H_

#include <string>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner.h"
#include "base/time/time.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_monster_change_dispatcher.h"
#include "net/cookies/cookie_store.h"
#include "xwalk/runtime/browser/android/net/xwalk_cookie_change_dispatcher_wrapper.h"

namespace xwalk {

// A cross-threaded cookie store implementation that wraps the CookieManager's
// CookieStore. It posts tasks to the CookieStore's thread, and invokes
// callbacks on the originating thread. Deleting it cancels pending callbacks.
// This is needed to allow Webview to run the CookieStore on its own thread, to
// enable synchronous calls into the store on the IO Thread from Java.
//
// XWalkCookieStoreWrapper will only grab the CookieStore pointer from the
// CookieManager when it's needed, allowing for lazy creation of the
// CookieStore.
//
// XWalkCookieStoreWrapper may only be called from the thread on which it's
// created.
//
// The CookieManager must outlive the XWalkCookieStoreWrapper.
class XWalkCookieStoreWrapper : public net::CookieStore {
 public:
  XWalkCookieStoreWrapper();
  ~XWalkCookieStoreWrapper() override;
  // CookieStore implementation:
  void SetCookieWithOptionsAsync(const GURL& url,
                                 const std::string& cookie_line,
                                 const net::CookieOptions& options,
                                 SetCookiesCallback callback) override;

  void SetCanonicalCookieAsync(std::unique_ptr<net::CanonicalCookie> cookie, std::string source_scheme,
                               const net::CookieOptions& options, SetCookiesCallback callback) override;
//  void GetCookiesWithOptionsAsync(const GURL& url,
//                                  const net::CookieOptions& options,
//                                  GetCookiesCallback callback) override;
  void GetCookieListWithOptionsAsync(
      const GURL& url,
      const net::CookieOptions& options,
      GetCookieListCallback callback) override;
  void GetAllCookiesAsync(GetCookieListCallback callback) override;
//  void DeleteCookieAsync(const GURL& url,
//                         const std::string& cookie_name,
//                         base::OnceClosure callback) override;
  void DeleteCanonicalCookieAsync(const net::CanonicalCookie& cookie,
                                  DeleteCallback callback) override;
  void DeleteAllCreatedInTimeRangeAsync(const net::CookieDeletionInfo::TimeRange& creation_range,
                                        DeleteCallback callback) override;
  void DeleteAllMatchingInfoAsync(net::CookieDeletionInfo delete_info,
                                  DeleteCallback callback) override;
  void DeleteSessionCookiesAsync(DeleteCallback callback) override;
  void FlushStore(base::OnceClosure callback) override;
  void SetForceKeepSessionState() override;
  net::CookieChangeDispatcher& GetChangeDispatcher() override;
  void SetCookieableSchemes(const std::vector<std::string>& schemes, SetCookieableSchemesCallback callback) override;

  void TriggerCookieFetch() override;

 private:
  // Used by CreateWrappedCallback below. Takes an arugment of Type and posts
  // a task to |task_runner| to invoke |callback| with that argument. If
  // |weak_cookie_store| is deleted before the task is run, the task will not
  // be run.
  template<class Type>
  static void RunCallbackOnClientThread(base::TaskRunner* task_runner,
                                        base::WeakPtr<XWalkCookieStoreWrapper> weak_cookie_store,
                                        base::OnceCallback<void(Type)> callback, Type argument) {
    task_runner->PostTask(
    FROM_HERE,
    base::BindOnce(&XWalkCookieStoreWrapper::RunClosureCallback,
        weak_cookie_store, base::BindOnce(std::move(callback), argument)));
  }

  // Returns a base::Callback that takes an argument of Type and posts a task to
  // the |client_task_runner_| to invoke |callback| with that argument.
  template <class Type>
  base::OnceCallback<void(Type)> CreateWrappedCallback( base::OnceCallback<void(Type)> callback) {
    if (callback.is_null())
    return callback;
    return base::BindOnce(&XWalkCookieStoreWrapper::RunCallbackOnClientThread<Type>,
    base::RetainedRef(client_task_runner_),
    weak_factory_.GetWeakPtr(), std::move(callback));
  }

  // These are the same as above, but specifically for GetCookieListCallback,
  // which has two arguments
  static void RunGetCookieListCallbackOnClientThread(
  base::TaskRunner* task_runner,
  base::WeakPtr<XWalkCookieStoreWrapper> weak_cookie_store,
  net::CookieStore::GetCookieListCallback callback,
  const net::CookieList& cookies,
  const net::CookieStatusList& excluded_cookies) {
    task_runner->PostTask(
    FROM_HERE,
    base::BindOnce(
        &XWalkCookieStoreWrapper::RunClosureCallback, weak_cookie_store,
        base::BindOnce(std::move(callback), cookies, excluded_cookies)));
  }
  net::CookieStore::GetCookieListCallback CreateWrappedGetCookieListCallback(
  net::CookieStore::GetCookieListCallback callback) {
    if (callback.is_null())
    return callback;
    return base::BindOnce(
    &XWalkCookieStoreWrapper::RunGetCookieListCallbackOnClientThread,
    base::RetainedRef(client_task_runner_), weak_factory_.GetWeakPtr(),
    std::move(callback));
  }

  // Returns a base::OnceClosure that posts a task to the |client_task_runner_|
  // to invoke |callback|.
  base::OnceClosure CreateWrappedClosureCallback(base::OnceClosure callback);

  // Runs |callback|. Used to prevent callbacks from being invoked after the
  // XWalkCookieStoreWrapper has been destroyed.
  void RunClosureCallback(base::OnceClosure callback);

  scoped_refptr<base::SingleThreadTaskRunner> client_task_runner_;
  XWalkCookieChangeDispatcherWrapper _change_dispatcher;
  base::WeakPtrFactory<XWalkCookieStoreWrapper> weak_factory_;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_NET_XWALK_COOKIE_STORE_WRAPPER_H_
