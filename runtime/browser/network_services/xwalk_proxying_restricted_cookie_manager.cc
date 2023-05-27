/*
 * xwalk_proxying_restricted_cookie_manager.cc
 *
 *  Created on: Dec 10, 2019
 *      Author: iotto
 */

#include "xwalk/runtime/browser/network_services/xwalk_proxying_restricted_cookie_manager.h"

#include "base/memory/ptr_util.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "xwalk/runtime/browser/android/xwalk_cookie_access_policy.h"

namespace xwalk {

class XwalkProxyingRestrictedCookieManagerListener
    : public network::mojom::CookieChangeListener {
 public:
  XwalkProxyingRestrictedCookieManagerListener(
      const GURL& url,
      const GURL& site_for_cookies,
      base::WeakPtr<XwalkProxyingRestrictedCookieManager>
          aw_restricted_cookie_manager,
      network::mojom::CookieChangeListenerPtr client_listener)
      : url_(url),
        site_for_cookies_(site_for_cookies),
        aw_restricted_cookie_manager_(aw_restricted_cookie_manager),
        client_listener_(std::move(client_listener)) {}

  void OnCookieChange(const net::CanonicalCookie& cookie,
                      network::mojom::CookieChangeCause cause) override {
    if (aw_restricted_cookie_manager_ &&
        aw_restricted_cookie_manager_->AllowCookies(url_, site_for_cookies_))
      client_listener_->OnCookieChange(cookie, cause);
  }

 private:
  const GURL url_;
  const GURL site_for_cookies_;
  base::WeakPtr<XwalkProxyingRestrictedCookieManager>
      aw_restricted_cookie_manager_;
  network::mojom::CookieChangeListenerPtr client_listener_;
};

// static
void XwalkProxyingRestrictedCookieManager::CreateAndBind(
    network::mojom::RestrictedCookieManagerPtrInfo underlying_rcm,
    bool is_service_worker,
    int process_id,
    int frame_id,
    network::mojom::RestrictedCookieManagerRequest request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(
          &XwalkProxyingRestrictedCookieManager::CreateAndBindOnIoThread,
          std::move(underlying_rcm), is_service_worker, process_id, frame_id,
          std::move(request)));
}

XwalkProxyingRestrictedCookieManager::~XwalkProxyingRestrictedCookieManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

void XwalkProxyingRestrictedCookieManager::GetAllForUrl(
    const GURL& url,
    const GURL& site_for_cookies,
    network::mojom::CookieManagerGetOptionsPtr options,
    GetAllForUrlCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->GetAllForUrl(
        url, site_for_cookies, std::move(options), std::move(callback));
  } else {
    std::move(callback).Run(std::vector<net::CanonicalCookie>());
  }
}

void XwalkProxyingRestrictedCookieManager::SetCanonicalCookie(
    const net::CanonicalCookie& cookie,
    const GURL& url,
    const GURL& site_for_cookies,
    SetCanonicalCookieCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->SetCanonicalCookie(
        cookie, url, site_for_cookies, std::move(callback));
  } else {
    std::move(callback).Run(false);
  }
}

void XwalkProxyingRestrictedCookieManager::AddChangeListener(
    const GURL& url,
    const GURL& site_for_cookies,
    network::mojom::CookieChangeListenerPtr listener,
    AddChangeListenerCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  network::mojom::CookieChangeListenerPtr proxy_listener_ptr;
  auto proxy_listener =
      std::make_unique<XwalkProxyingRestrictedCookieManagerListener>(
          url, site_for_cookies, weak_factory_.GetWeakPtr(),
          std::move(listener));

  mojo::MakeStrongBinding(std::move(proxy_listener),
                          mojo::MakeRequest(&proxy_listener_ptr));

  underlying_restricted_cookie_manager_->AddChangeListener(
      url, site_for_cookies, std::move(proxy_listener_ptr),
      std::move(callback));
}

void XwalkProxyingRestrictedCookieManager::SetCookieFromString(
    const GURL& url,
    const GURL& site_for_cookies,
    const std::string& cookie,
    SetCookieFromStringCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->SetCookieFromString(
        url, site_for_cookies, cookie, std::move(callback));
  } else {
    std::move(callback).Run();
  }
}

void XwalkProxyingRestrictedCookieManager::GetCookiesString(
    const GURL& url,
    const GURL& site_for_cookies,
    GetCookiesStringCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->GetCookiesString(
        url, site_for_cookies, std::move(callback));
  } else {
    std::move(callback).Run("");
  }
}

void XwalkProxyingRestrictedCookieManager::CookiesEnabledFor(
    const GURL& url,
    const GURL& site_for_cookies,
    CookiesEnabledForCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::move(callback).Run(AllowCookies(url, site_for_cookies));
}

XwalkProxyingRestrictedCookieManager::XwalkProxyingRestrictedCookieManager(
    network::mojom::RestrictedCookieManagerPtr
        underlying_restricted_cookie_manager,
    bool is_service_worker,
    int process_id,
    int frame_id)
    : underlying_restricted_cookie_manager_(
          std::move(underlying_restricted_cookie_manager)),
      is_service_worker_(is_service_worker),
      process_id_(process_id),
      frame_id_(frame_id),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

// static
void XwalkProxyingRestrictedCookieManager::CreateAndBindOnIoThread(
    network::mojom::RestrictedCookieManagerPtrInfo underlying_rcm,
    bool is_service_worker,
    int process_id,
    int frame_id,
    network::mojom::RestrictedCookieManagerRequest request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  auto wrapper = base::WrapUnique(new XwalkProxyingRestrictedCookieManager(
      network::mojom::RestrictedCookieManagerPtr(std::move(underlying_rcm)),
      is_service_worker, process_id, frame_id));
  mojo::MakeStrongBinding(std::move(wrapper), std::move(request));
}

bool XwalkProxyingRestrictedCookieManager::AllowCookies(
    const GURL& url,
    const GURL& site_for_cookies) const {
  // see: android_webview/browser/network_service/aw_proxying_restricted_cookie_manager.cc
  if (is_service_worker_) {
    // Service worker cookies are always first-party, so only need to check
    // the global toggle.
    return XWalkCookieAccessPolicy::GetInstance()->GetShouldAcceptCookies();
  } else {
    return XWalkCookieAccessPolicy::GetInstance()->AllowCookies(
        url, site_for_cookies, process_id_, frame_id_);
  }
}

} /* namespace xwalk */
