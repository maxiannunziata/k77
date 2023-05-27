// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/net/xwalk_cookie_store_wrapper.h"

#include <string>

#include "base/bind.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "url/gurl.h"
#include "xwalk/runtime/browser/android/net/init_native_callback.h"

namespace xwalk {

namespace {

// Posts |task| to the thread that the global CookieStore lives on.
void PostTaskToCookieStoreTaskRunner(base::OnceClosure task) {
  GetCookieStoreTaskRunner()->PostTask(FROM_HERE, std::move(task));
}

void SetCookieWithOptionsAsyncOnCookieThread(
    const GURL& url,
    const std::string& cookie_line,
    const net::CookieOptions& options,
    net::CookieStore::SetCookiesCallback callback) {
  DLOG(INFO) << "Setting cookie Async in cookie_store_wrapper";
  GetCookieStore()->SetCookieWithOptionsAsync(url, cookie_line, options,
                                              std::move(callback));
}

void SetCanonicalCookieAsyncOnCookieThread(std::unique_ptr<net::CanonicalCookie> cookie, std::string source_scheme,
                                           const net::CookieOptions& options, net::CookieStore::SetCookiesCallback callback) {

  GetCookieStore()->SetCanonicalCookieAsync(std::move(cookie), source_scheme, options, std::move(callback));
}

void GetCookieListWithOptionsAsyncOnCookieThread(
    const GURL& url,
    const net::CookieOptions& options,
    net::CookieStore::GetCookieListCallback callback) {
  GetCookieStore()->GetCookieListWithOptionsAsync(url, options, std::move(callback));
}

void GetAllCookiesAsyncOnCookieThread(
    net::CookieStore::GetCookieListCallback callback) {
  GetCookieStore()->GetAllCookiesAsync(std::move(callback));
}

void DeleteCanonicalCookieAsyncOnCookieThread(
    const net::CanonicalCookie& cookie,
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteCanonicalCookieAsync(cookie, std::move(callback));
}

void DeleteAllCreatedInTimeRangeAsyncOnCookieThread(
    const net::CookieDeletionInfo::TimeRange& creation_range,
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteAllCreatedInTimeRangeAsync(creation_range,
                                                 std::move(callback));
}

void DeleteAllMatchingInfoAsyncOnCookieThread(net::CookieDeletionInfo delete_info, net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteAllMatchingInfoAsync(delete_info, std::move(callback));
}

void DeleteSessionCookiesAsyncOnCookieThread(
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteSessionCookiesAsync(std::move(callback));
}

void SetCookieableSchemesOnCookieThread(const std::vector<std::string>& schemes,
                                        net::CookieStore::SetCookieableSchemesCallback callback) {
  GetCookieStore()->SetCookieableSchemes(schemes, std::move(callback));
}

void FlushStoreOnCookieThread(base::OnceClosure callback) {
  GetCookieStore()->FlushStore(std::move(callback));
}

void SetForceKeepSessionStateOnCookieThread() {
  GetCookieStore()->SetForceKeepSessionState();
}

void TriggerCookieFetchOnCookieThread() {
  GetCookieStore()->TriggerCookieFetch();
}

}  // namespace

XWalkCookieStoreWrapper::XWalkCookieStoreWrapper()
    : client_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {}

XWalkCookieStoreWrapper::~XWalkCookieStoreWrapper() {}

void XWalkCookieStoreWrapper::SetCookieWithOptionsAsync(
    const GURL& url,
    const std::string& cookie_line,
    const net::CookieOptions& options,
    net::CookieStore::SetCookiesCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&SetCookieWithOptionsAsyncOnCookieThread, url, cookie_line, options,
                     CreateWrappedCallback<net::CanonicalCookie::CookieInclusionStatus>(std::move(callback))));
}

/**
 *
 */
void XWalkCookieStoreWrapper::SetCanonicalCookieAsync(std::unique_ptr<net::CanonicalCookie> cookie, std::string source_scheme,
                                                      const net::CookieOptions& options, SetCookiesCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&SetCanonicalCookieAsyncOnCookieThread, std::move(cookie), source_scheme, options,
                     CreateWrappedCallback<net::CanonicalCookie::CookieInclusionStatus>(std::move(callback))));
}

void XWalkCookieStoreWrapper::GetCookieListWithOptionsAsync(const GURL& url, const net::CookieOptions& options,
                                                            GetCookieListCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&GetCookieListWithOptionsAsyncOnCookieThread, url, options,
                     CreateWrappedGetCookieListCallback(std::move(callback))));
}

void XWalkCookieStoreWrapper::GetAllCookiesAsync(
    GetCookieListCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&GetAllCookiesAsyncOnCookieThread, CreateWrappedGetCookieListCallback(std::move(callback))));
}

void XWalkCookieStoreWrapper::DeleteCanonicalCookieAsync(
    const net::CanonicalCookie& cookie,
    DeleteCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteCanonicalCookieAsyncOnCookieThread, cookie,
                     CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void XWalkCookieStoreWrapper::DeleteAllCreatedInTimeRangeAsync(const net::CookieDeletionInfo::TimeRange& creation_range,
                                                               net::CookieStore::DeleteCallback callback) {

  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteAllCreatedInTimeRangeAsyncOnCookieThread, creation_range,
                     CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void XWalkCookieStoreWrapper::DeleteAllMatchingInfoAsync(net::CookieDeletionInfo delete_info, DeleteCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteAllMatchingInfoAsyncOnCookieThread, std::move(delete_info),
                     CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void XWalkCookieStoreWrapper::DeleteSessionCookiesAsync(
    DeleteCallback callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteSessionCookiesAsyncOnCookieThread, CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void XWalkCookieStoreWrapper::FlushStore(base::OnceClosure callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&FlushStoreOnCookieThread, CreateWrappedClosureCallback(std::move(callback))));
}

void XWalkCookieStoreWrapper::SetForceKeepSessionState() {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&SetForceKeepSessionStateOnCookieThread));
}

net::CookieChangeDispatcher& XWalkCookieStoreWrapper::GetChangeDispatcher() {
  return _change_dispatcher;
}

void XWalkCookieStoreWrapper::SetCookieableSchemes(const std::vector<std::string>& schemes,
                                                   SetCookieableSchemesCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&SetCookieableSchemesOnCookieThread, schemes, CreateWrappedCallback<bool>(std::move(callback))));
}

void XWalkCookieStoreWrapper::TriggerCookieFetch() {
  PostTaskToCookieStoreTaskRunner(base::Bind(&TriggerCookieFetchOnCookieThread));
}

base::OnceClosure XWalkCookieStoreWrapper::CreateWrappedClosureCallback(base::OnceClosure callback) {
  if (callback.is_null())
    return callback;
  return base::BindOnce(base::IgnoreResult(&base::TaskRunner::PostTask), client_task_runner_, FROM_HERE,
  base::BindOnce(&XWalkCookieStoreWrapper::RunClosureCallback,
      weak_factory_.GetWeakPtr(), std::move(callback)));
}

void XWalkCookieStoreWrapper::RunClosureCallback(base::OnceClosure callback) {
  DCHECK(client_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run();
}

}  // namespace xwalk
