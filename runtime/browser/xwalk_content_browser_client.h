// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_XWALK_CONTENT_BROWSER_CLIENT_H_
#define XWALK_RUNTIME_BROWSER_XWALK_CONTENT_BROWSER_CLIENT_H_

#include <map>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/scoped_file.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/main_function_params.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "xwalk/runtime/browser/runtime_resource_dispatcher_host_delegate.h"

namespace content {
class BrowserContext;
class ResourceContext;
class RenderViewHost;
class QuotaPermissionContext;
class SpeechRecognitionManagerDelegate;
class WebContents;
class WebContentsViewDelegate;
class PresentationServiceDelegate;
struct WebPreferences;
}

namespace net {
class URLRequestContextGetter;
}

namespace xwalk {

class XWalkBrowserContext;
class XWalkBrowserMainParts;
class XWalkRunner;
class XWalkDevToolsManagerDelegate;

class XWalkContentBrowserClient : public content::ContentBrowserClient {
 public:
  // TODO(iotto): Get rid of this, shoudn't be static
  static XWalkContentBrowserClient* Get();

  // This is what AwContentBrowserClient::GetAcceptLangs uses.
  static std::string GetAcceptLangsImpl();

  // Called by WebContents to override the WebKit preferences that are used by
  // the renderer. The content layer will add its own settings, and then it's up
  // to the embedder to update it if it wants.
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                                   content::WebPreferences* prefs) override;

  explicit XWalkContentBrowserClient(XWalkRunner* xwalk_runner);
  ~XWalkContentBrowserClient() override;

  // ContentBrowserClient overrides.
  network::mojom::NetworkContextPtr CreateNetworkContext(content::BrowserContext* context, bool in_memory,
                                                         const base::FilePath& relative_partition_path) override;
  std::unique_ptr<content::BrowserMainParts> CreateBrowserMainParts(const content::MainFunctionParams& parameters)
      override;
  void RunServiceInstance(const service_manager::Identity& identity,
                          mojo::PendingReceiver<service_manager::mojom::Service>* receiver) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line, int child_process_id) override;
  std::string GetApplicationLocale() override;
  std::string GetAcceptLangs(content::BrowserContext* context) override;
  scoped_refptr<content::QuotaPermissionContext> CreateQuotaPermissionContext() override;
  content::GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(content::BrowserContext* context) override;
  content::WebContentsViewDelegate* GetWebContentsViewDelegate(content::WebContents* web_contents) override;
  void RenderProcessWillLaunch(content::RenderProcessHost* host,
                               service_manager::mojom::ServiceRequest* service_request) override;
  bool IsHandledURL(const GURL& url) override;
  content::MediaObserver* GetMediaObserver() override;
  base::Optional<service_manager::Manifest> GetServiceManifestOverlay(base::StringPiece name) override;
  void BindInterfaceRequestFromFrame(content::RenderFrameHost* render_frame_host, const std::string& interface_name,
                                     mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool BindAssociatedInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host, const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle* handle) override;
  bool WillCreateURLLoaderFactory(content::BrowserContext* browser_context, content::RenderFrameHost* frame,
                                  int render_process_id, bool is_navigation, bool is_download,
                                  const url::Origin& request_initiator,
                                  mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
                                  network::mojom::TrustedURLLoaderHeaderClientPtrInfo* header_client,
                                  bool* bypass_redirect_checks) override;
  bool WillCreateRestrictedCookieManager(
      network::mojom::RestrictedCookieManagerRole role, content::BrowserContext* browser_context,
      const url::Origin& origin,
      bool is_service_worker,
      int process_id, int routing_id, network::mojom::RestrictedCookieManagerRequest* request) override;

  void AllowCertificateError(content::WebContents* web_contents, int cert_error, const net::SSLInfo& ssl_info,
                             const GURL& request_url,
                             bool is_main_frame_request,
                             bool strict_enforcement,
                             bool expired_previous_decision,
                             const base::Callback<void(content::CertificateRequestResultType)>& callback) override;

  base::OnceClosure SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  content::SpeechRecognitionManagerDelegate*
      CreateSpeechRecognitionManagerDelegate() override;

  content::PlatformNotificationService* GetPlatformNotificationService(content::BrowserContext* browser_context)
      override;

#if !defined(OS_ANDROID)
  bool CanCreateWindow(const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       WindowOpenDisposition disposition,
                       const blink::WebWindowFeatures& features,
                       bool user_gesture,
                       bool opener_suppressed,
                       content::ResourceContext* context,
                       int render_process_id,
                       int opener_render_view_id,
                       int opener_render_frame_id,
                       bool* no_javascript_access) override;
#endif

  void DidCreatePpapiPlugin(
      content::BrowserPpapiHost* browser_host) override;
  content::BrowserPpapiHost* GetExternalBrowserPpapiHost(
      int plugin_process_id) override;

#if defined(OS_ANDROID) || defined(OS_LINUX)
  void ResourceDispatcherHostCreated() override;
#endif

  void GetStoragePartitionConfigForSite(
      content::BrowserContext* browser_context,
      const GURL& site,
      bool can_be_default,
      std::string* partition_domain,
      std::string* partition_name,
      bool* in_memory) override;

  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;

  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;
  void OpenURL(content::SiteInstance* site_instance,
               const content::OpenURLParams& params,
               base::OnceCallback<void(content::WebContents*)> callback) override;
#if defined(OS_ANDROID)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::PosixFileDescriptorInfo* mappings) override;
#elif defined(OS_POSIX) && !defined(OS_MACOSX)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::FileDescriptorInfo* mappings) override {}
#endif

  XWalkBrowserMainParts* main_parts() { return main_parts_; }

  content::ControllerPresentationServiceDelegate*
    GetControllerPresentationServiceDelegate(content::WebContents* web_contents) override;

#if defined(OS_ANDROID)
  RuntimeResourceDispatcherHostDelegate* resource_dispatcher_host_delegate() {
//    return nullptr;
    return resource_dispatcher_host_delegate_.get();
  }
#endif

  std::string GetProduct() override;
  std::string GetUserAgent() override;

#if defined(OS_ANDROID)
  std::vector<std::unique_ptr<content::NavigationThrottle>> CreateThrottlesForNavigation(
      content::NavigationHandle* navigation_handle) override;
#endif

  std::unique_ptr<content::LoginDelegate> CreateLoginDelegate(const net::AuthChallengeInfo& auth_info,
                                                              content::WebContents* web_contents,
                                                              const content::GlobalRequestID& request_id,
                                                              bool is_main_frame,
                                                              const GURL& url,
                                                              scoped_refptr<net::HttpResponseHeaders> response_headers,
                                                              bool first_auth_attempt,
                                                              LoginAuthRequiredCallback auth_required_callback)
                                                                  override;
  bool HandleExternalProtocol(const GURL& url, content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
                              int child_id, content::NavigationUIData* navigation_data, bool is_main_frame,
                              ui::PageTransition page_transition, bool has_user_gesture,
                              network::mojom::URLLoaderFactoryPtr* out_factory) override;
  void RegisterNonNetworkSubresourceURLLoaderFactories(int render_process_id, int render_frame_id,
                                                       NonNetworkURLLoaderFactoryMap* factories) override;

  // TODO(iotto) : Implement for geolocation
//  // Allows the embedder to provide a URLRequestContextGetter to use for network
//  // geolocation queries.
//  // * May be called from any thread. A URLRequestContextGetter is then provided
//  //   by invoking |callback| on the calling thread.
//  // * Default implementation provides nullptr URLRequestContextGetter.
//  virtual void GetGeolocationRequestContext(
//      base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
//          callback);

  // TODO (iotto) : use the method to register autofill interfaces
//  void RegisterRenderFrameMojoInterfaces(
//      service_manager::InterfaceRegistry* registry,
//      content::RenderFrameHost* render_frame_host) override;

  //**************** private ************
 private:
  virtual void ExposeInterfacesToFrame(service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>* registry);

  XWalkRunner* xwalk_runner_;
  std::unique_ptr<content::ClientCertificateDelegate> delegate_;
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  base::ScopedFD v8_natives_fd_;
  base::ScopedFD v8_snapshot_fd_;
#endif

  XWalkBrowserMainParts* main_parts_;

//  std::unique_ptr<content::ResourceDispatcherHostDelegate>
//      resource_dispatcher_host_delegate_;
  std::unique_ptr<RuntimeResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;

  std::unique_ptr<service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>> frame_interfaces_;

  DISALLOW_COPY_AND_ASSIGN(XWalkContentBrowserClient);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_XWALK_CONTENT_BROWSER_CLIENT_H_
