// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/xwalk_content_browser_client.h"

#include <string>
#include <vector>
#include <memory>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/nacl/common/buildflags.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/file_url_loader.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/shared_cors_origin_access_list.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "gin/v8_initializer.h"
#include "gin/public/isolate_holder.h"
#include "media/mojo/buildflags.h"
#include "net/ssl/ssl_info.h"
#include "net/url_request/url_request_context_getter.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/buildflags/buildflags.h"
#include "services/device/geolocation/geolocation_provider.h"
#include "services/network/public/cpp/features.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "xwalk/extensions/common/xwalk_extension_switches.h"
#include "xwalk/application/common/constants.h"
#include "xwalk/runtime/browser/media/media_capture_devices_dispatcher.h"
#include "xwalk/runtime/browser/renderer_host/pepper/xwalk_browser_pepper_host_factory.h"
#include "xwalk/runtime/browser/runtime_platform_util.h"
#include "xwalk/runtime/browser/runtime_quota_permission_context.h"
#include "xwalk/runtime/browser/ssl_error_page.h"
#include "xwalk/runtime/browser/speech/speech_recognition_manager_delegate.h"
#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/browser/xwalk_browser_main_parts.h"
#include "xwalk/runtime/browser/xwalk_content_overlay_manifests.h"
#include "xwalk/runtime/browser/xwalk_platform_notification_service.h"
#include "xwalk/runtime/browser/xwalk_render_message_filter.h"
#include "xwalk/runtime/browser/xwalk_runner.h"
#include "xwalk/runtime/common/xwalk_paths.h"
#include "xwalk/runtime/common/xwalk_switches.h"
#include "xwalk/runtime/browser/devtools/xwalk_devtools_manager_delegate.h"
#include "xwalk/runtime/browser/network_services/xwalk_proxying_restricted_cookie_manager.h"
#include "xwalk/runtime/browser/network_services/xwalk_proxying_url_loader_factory.h"

#if BUILDFLAG(ENABLE_NACL)
#include "components/nacl/browser/nacl_browser.h"
#include "components/nacl/browser/nacl_host_message_filter.h"
#include "components/nacl/browser/nacl_process_host.h"
#include "components/nacl/common/nacl_process_type.h"
#endif

#if defined(OS_ANDROID)
#include "base/android/apk_assets.h"
#include "base/android/locale_utils.h"
#include "base/android/path_utils.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/base_paths_android.h"
#include "components/cdm/browser/cdm_message_filter_android.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "xwalk/runtime/browser/android/net/url_constants.h"
#include "xwalk/runtime/browser/android/xwalk_http_auth_handler.h"
#include "xwalk/runtime/browser/android/xwalk_cookie_access_policy.h"
#include "xwalk/runtime/browser/android/xwalk_content.h"
#include "xwalk/runtime/browser/android/xwalk_contents_client_bridge.h"
#include "xwalk/runtime/browser/android/xwalk_settings.h"
#include "xwalk/runtime/browser/android/xwalk_web_contents_view_delegate.h"
#include "xwalk/runtime/browser/xwalk_browser_main_parts_android.h"
#include "xwalk/runtime/common/android/xwalk_globals_android.h"
#include "xwalk/runtime/common/android/xwalk_descriptors.h"
#else
#include "xwalk/application/browser/application_system.h"
#include "xwalk/application/browser/application_service.h"
#include "xwalk/application/browser/application.h"
#endif

#if defined(OS_MACOSX)
#include "xwalk/runtime/browser/xwalk_browser_main_parts_mac.h"
#endif

#if defined(OS_ANDROID)
//#include "xwalk/runtime/browser/xwalk_presentation_service_delegate_android.h"
#elif defined(OS_WIN)
#include "xwalk/runtime/browser/xwalk_presentation_service_delegate_win.h"
#endif

#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS)
#include "media/mojo/interfaces/constants.mojom.h"      // nogncheck
#include "media/mojo/services/media_service_factory.h"  // nogncheck
#endif

#include "meta_logging.h"

using content::WebContents;

namespace xwalk {

namespace {

// The application-wide singleton of ContentBrowserClient impl.
XWalkContentBrowserClient* g_browser_client = nullptr;

//void PassMojoCookieManagerToAwCookieManager(
//    const network::mojom::NetworkContextPtr& network_context) {
//  // Get the CookieManager from the NetworkContext.
//  network::mojom::CookieManagerPtrInfo cookie_manager_info;
//  network_context->GetCookieManager(mojo::MakeRequest(&cookie_manager_info));
//
//  // Pass the CookieManagerPtrInfo to android_webview::CookieManager, so it can
//  // implement its APIs with this mojo CookieManager.
//  CookieManager::GetInstance()->SetMojoCookieManager(
//      std::move(cookie_manager_info));
//}

} //

  std::string GetProduct() {
    return version_info::GetProductNameAndVersionForUserAgent();
  }

std::string GetUserAgent() {
  std::string product = GetProduct();
  product += " Crosswalk/" XWALK_VERSION;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kUseMobileUserAgent))
    product += " Mobile";
  return content::BuildUserAgentFromProduct(product);
}

// static
XWalkContentBrowserClient* XWalkContentBrowserClient::Get() {
  return g_browser_client;
}

// TODO(yirui): can use similar logic as in PrependToAcceptLanguagesIfNecessary
// in chrome/browser/android/preferences/pref_service_bridge.cc
// static
std::string XWalkContentBrowserClient::GetAcceptLangsImpl() {
  // Start with the current locale(s) in BCP47 format.
  std::string locales_string = XWalkContent::GetLocaleList();

  // If accept languages do not contain en-US, add in en-US which will be
  // used with a lower q-value.
  if (locales_string.find("en-US") == std::string::npos)
    locales_string += ",en-US";
  return locales_string;
}

XWalkContentBrowserClient::XWalkContentBrowserClient(XWalkRunner* xwalk_runner)
    : xwalk_runner_(xwalk_runner),
#if defined(OS_POSIX) && !defined(OS_MACOSX)
      v8_natives_fd_(-1),
      v8_snapshot_fd_(-1),
#endif  // OS_POSIX && !OS_MACOSX
      main_parts_(nullptr) {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

XWalkContentBrowserClient::~XWalkContentBrowserClient() {
  DCHECK(g_browser_client);
  g_browser_client = nullptr;
}

void XWalkContentBrowserClient::OverrideWebkitPrefs(content::RenderViewHost* renderViewHost,
    content::WebPreferences* webPrefs) {
  XWalkSettings* settings = XWalkSettings::FromWebContents(
      content::WebContents::FromRenderViewHost(renderViewHost));
  if (settings) {
    settings->PopulateWebPreferences(webPrefs);
  }

  return;
}

#if defined(OS_ANDROID)
void XWalkContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::PosixFileDescriptorInfo* mappings) {

  base::MemoryMappedFile::Region region;
  int fd = ui::GetMainAndroidPackFd(&region);
  mappings->ShareWithRegion(kXWalkMainPakDescriptor, fd, region);

  fd = ui::GetCommonResourcesPackFd(&region);
  mappings->ShareWithRegion(kXWalk100PakDescriptor, fd, region);

  fd = ui::GetLocalePackFd(&region);
  mappings->ShareWithRegion(kXWalkLocalePakDescriptor, fd, region);
}
#endif //defined(OS_ANDROID)

network::mojom::NetworkContextPtr
XWalkContentBrowserClient::CreateNetworkContext(content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService))
    return nullptr;

  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      XWalkRunner::GetInstance()->CreateHttpAuthDynamicParams());

  auto* aw_context = static_cast<XWalkBrowserContext*>(context);
  network::mojom::NetworkContextPtr network_context;
  network::mojom::NetworkContextParamsPtr context_params =
      aw_context->GetNetworkContextParams(in_memory, relative_partition_path);
  content::GetNetworkService()->CreateNetworkContext(
      MakeRequest(&network_context), std::move(context_params));
//
//  // Pass a CookieManager to the code supporting AwCookieManager.java (i.e., the
//  // Cookies APIs).
//  PassMojoCookieManagerToAwCookieManager(network_context);

  return network_context;
}

std::unique_ptr<content::BrowserMainParts> XWalkContentBrowserClient::CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) {
  std::unique_ptr<XWalkBrowserMainParts> ret_val;

#if defined(OS_MACOSX)
  main_parts_ = new XWalkBrowserMainPartsMac(parameters);
#elif defined(OS_ANDROID)
  ret_val = base::WrapUnique(new XWalkBrowserMainPartsAndroid(parameters));
#else
  main_parts_ = new XWalkBrowserMainParts(parameters);
#endif

  main_parts_ = ret_val.get();
  return ret_val;
}

void XWalkContentBrowserClient::RunServiceInstance(const service_manager::Identity& identity,
    mojo::PendingReceiver<service_manager::mojom::Service>* receiver) {
  LOG(INFO) << "iotto " << __func__ << " identity=" << identity.name();
#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS)
  LOG(INFO) << "iotto " << __func__ << " running identity=" << identity.name();
  if (identity.name() == media::mojom::kMediaServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        media::CreateMediaService(std::move(*receiver)));
  }
#endif
}

// This allow us to append extra command line switches to the child
// process we launch.
void XWalkContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line, int child_process_id) {
//  const base::CommandLine& browser_process_cmd_line =
//      *base::CommandLine::ForCurrentProcess();
//  const char* extra_switches[] = {
////    switches::kXWalkDisableExtensionProcess,
//#if BUILDFLAG(ENABLE_PLUGINS)
//    switches::kPpapiFlashPath,
//    switches::kPpapiFlashVersion
//#endif
//  };
//
//  command_line->CopySwitchesFrom(
//      browser_process_cmd_line, extra_switches, base::size(extra_switches));
}

std::string XWalkContentBrowserClient::GetApplicationLocale() {
  #if defined(OS_ANDROID)
    return base::android::GetDefaultLocaleString();
  #else
    return content::ContentBrowserClient::GetApplicationLocale();
  #endif
}
std::string XWalkContentBrowserClient::GetAcceptLangs(content::BrowserContext* context) {
  // TODO(iotto): Maybe handle for other than ANDROID
  return GetAcceptLangsImpl();
}

scoped_refptr<content::QuotaPermissionContext>
XWalkContentBrowserClient::CreateQuotaPermissionContext() {
  return new RuntimeQuotaPermissionContext();
}

content::GeneratedCodeCacheSettings
XWalkContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  // If we pass 0 for size, disk_cache will pick a default size using the
  // heuristics based on available disk size. These are implemented in
  // disk_cache::PreferredCacheSize in net/disk_cache/cache_util.cc.
//  return content::GeneratedCodeCacheSettings(true, 0, context->GetPath());
  base::FilePath path(context->GetPath());
  // TODO(iotto): disabled Code Cache
  return content::GeneratedCodeCacheSettings(false, 0, path);
}

content::WebContentsViewDelegate*
XWalkContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
#if defined(OS_ANDROID)
  return new XWalkWebContentsViewDelegate(web_contents);
#else
  return nullptr;
#endif
}

void XWalkContentBrowserClient::RenderProcessWillLaunch(content::RenderProcessHost* host,
      service_manager::mojom::ServiceRequest* service_request) {
#if BUILDFLAG(ENABLE_NACL)
  int id = host->GetID();
  net::URLRequestContextGetter* context =
      host->GetStoragePartition()->GetURLRequestContext();

  host->AddFilter(new nacl::NaClHostMessageFilter(
      id,
      // TODO(Halton): IsOffTheRecord?
      false,
      host->GetBrowserContext()->GetPath(),
      context));
#endif
  xwalk_runner_->OnRenderProcessWillLaunch(host);
  host->AddFilter(new XWalkRenderMessageFilter);
#if defined(OS_ANDROID)
  host->AddFilter(new cdm::CdmMessageFilterAndroid(true, false));
  host->AddFilter(new XWalkRenderMessageFilter(host->GetID()));
#endif
}

bool XWalkContentBrowserClient::IsHandledURL(const GURL& url) {
  LOG(INFO) << "iotto " << __func__ << " url=" << url;
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }

  const std::string scheme = url.scheme();
  // See CreateJobFactory in aw_url_request_context_getter.cc for the
  // list of protocols that are handled.
  // TODO(mnaganov): Make this automatic.
  static const char* const kProtocolList[] = {
    url::kDataScheme,
    url::kBlobScheme,
    url::kFileSystemScheme,
//    content::kChromeUIScheme,
    url::kContentScheme,
  };
  if (scheme == url::kFileScheme) {
    // Return false for the "special" file URLs, so they can be loaded
    // even if access to file: scheme is not granted to the child process.
    bool ret_val = !IsAndroidSpecialFileUrl(url);
    LOG(INFO) << "iotto " << __func__ << " isHandledFileScheme=" << ret_val << " url=" << url;
    return true;
  }
  for (size_t i = 0; i < base::size(kProtocolList); ++i) {
    if (scheme == kProtocolList[i])
      return true;
  }
  return net::URLRequest::IsHandledProtocol(scheme);
}

content::MediaObserver* XWalkContentBrowserClient::GetMediaObserver() {
  return XWalkMediaCaptureDevicesDispatcher::GetInstance();
}

// Selects a SSL client certificate and returns it to the |delegate|. If no
// certificate was selected NULL is returned to the |delegate|.
base::OnceClosure XWalkContentBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
#if defined(OS_ANDROID)
  XWalkContentsClientBridge* client =
      XWalkContentsClientBridge::FromWebContents(web_contents);
  if (client) {
    client->SelectClientCertificate(cert_request_info, std::move(delegate));
  } else {
    delegate->ContinueWithCertificate(nullptr, nullptr);
  }
#endif
  return base::OnceClosure();
}

void XWalkContentBrowserClient::AllowCertificateError(content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool is_main_frame_request,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>& callback) {
  TENTA_LOG(ERROR) << __func__ << " error=" << cert_error << " url=" << request_url;
  // Currently only Android handles it.
  // TODO(yongsheng): applies it for other platforms?
#if defined(OS_ANDROID)
  XWalkContentsClientBridge* client =
      XWalkContentsClientBridge::FromWebContents(web_contents);
  bool cancel_request = true;
  if (client)
    client->AllowCertificateError(cert_error,
                                  ssl_info.cert.get(),
                                  request_url,
                                  callback, &cancel_request);

  if (cancel_request)
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
#else
  DCHECK(web_contents);
  // The interstitial page shown is responsible for destroying
  // this instance of SSLErrorPage
  (new SSLErrorPage(web_contents, cert_error,
                    ssl_info, request_url, callback))->Show();
#endif
}

content::PlatformNotificationService*
XWalkContentBrowserClient::GetPlatformNotificationService(content::BrowserContext* browser_context) {
  LOG(ERROR) << "iotto " << __func__ << " IMPLEMENT";
//  return XWalkPlatformNotificationService::GetInstance();
  return nullptr;
}

void XWalkContentBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* browser_host) {
#if BUILDFLAG(ENABLE_PLUGINS)
  browser_host->GetPpapiHost()->AddHostFactoryFilter(
      std::unique_ptr<ppapi::host::HostFactory>(
          new XWalkBrowserPepperHostFactory(browser_host)));
#endif
}

content::BrowserPpapiHost*
    XWalkContentBrowserClient::GetExternalBrowserPpapiHost(
        int plugin_process_id) {
#if BUILDFLAG(ENABLE_NACL)
  content::BrowserChildProcessHostIterator iter(PROCESS_TYPE_NACL_LOADER);
  while (!iter.Done()) {
    nacl::NaClProcessHost* host = static_cast<nacl::NaClProcessHost*>(
        iter.GetDelegate());
    if (host->process() &&
        host->process()->GetData().id == plugin_process_id) {
      // Found the plugin.
      return host->browser_ppapi_host();
    }
    ++iter;
  }
#endif
  return nullptr;
}

#if defined(OS_ANDROID) || defined(OS_LINUX)
void XWalkContentBrowserClient::ResourceDispatcherHostCreated() {
//  resource_dispatcher_host_delegate_.reset(
//      new content::ResourceDispatcherHostDelegate());
//  content::ResourceDispatcherHost::Get()->SetDelegate(
//      resource_dispatcher_host_delegate_.get());
//
//  LOG(WARNING) << "iotto " << __func__ << " reinstate ResourceDispatcherHostDelegate";

  resource_dispatcher_host_delegate_ =
      (RuntimeResourceDispatcherHostDelegate::Create());
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}
#endif

content::SpeechRecognitionManagerDelegate*
    XWalkContentBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return nullptr;
//  return new xwalk::XWalkSpeechRecognitionManagerDelegate();
}

#if !defined(OS_ANDROID)
bool XWalkContentBrowserClient::CanCreateWindow(const GURL& opener_url,
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
                             bool* no_javascript_access) {
  *no_javascript_access = false;
  application::Application* app = xwalk_runner_->app_system()->
      application_service()->GetApplicationByRenderHostID(render_process_id);
  if (!app)
    // If it's not a request from an application, always enable this action.
    return true;

  if (app->CanRequestURL(target_url)) {
    LOG(INFO) << "[ALLOW] CreateWindow: " << target_url.spec();
    return true;
  }

  LOG(INFO) << "[BlOCK] CreateWindow: " << target_url.spec();
  platform_util::OpenExternal(target_url);
  return false;
}
#endif

void XWalkContentBrowserClient::GetStoragePartitionConfigForSite(
    content::BrowserContext* browser_context,
    const GURL& site,
    bool can_be_default,
    std::string* partition_domain,
    std::string* partition_name,
    bool* in_memory) {
  *in_memory = false;
  partition_domain->clear();
  partition_name->clear();

#if !defined(OS_ANDROID)
  if (site.SchemeIs(application::kApplicationScheme))
    *partition_domain = site.host();
#endif
}

void XWalkContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
#if !defined(OS_ANDROID)
  additional_schemes->push_back("app");
#endif
}

content::DevToolsManagerDelegate* 
XWalkContentBrowserClient::GetDevToolsManagerDelegate() {
  // called from DevToolsManager::DevToolsManager and stored as uniqe_ptr
  return new XWalkDevToolsManagerDelegate(xwalk_runner_->browser_context());
}

void XWalkContentBrowserClient::OpenURL(content::SiteInstance* site_instance,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::WebContents*)> callback) {

  LOG(WARNING) << "iotto " << __func__ << " CHECK/IMPLEMENT";
  std::move(callback).Run(nullptr);
}

content::ControllerPresentationServiceDelegate* XWalkContentBrowserClient::
    GetControllerPresentationServiceDelegate(content::WebContents* web_contents) {

  LOG(WARNING) << "iotto " << __func__ << " FIX PresentationServiceDelegate";
  return nullptr;
////  if (media_router::MediaRouterEnabled(web_contents->GetBrowserContext())) {
////    return media_router::PresentationServiceDelegateImpl::
////        GetOrCreateForWebContents(web_contents);
////  }
////  return nullptr;
//
//#if defined(OS_WIN)
//  return XWalkPresentationServiceDelegateWin::
//      GetOrCreateForWebContents(web_contents);
//#elif defined(OS_ANDROID)
//  return XWalkPresentationServiceDelegateAndroid::
//      GetOrCreateForWebContents(web_contents);
//#else
//  return nullptr;
//#endif
}


std::string XWalkContentBrowserClient::GetProduct() {
  return xwalk::GetProduct();
}

std::string XWalkContentBrowserClient::GetUserAgent() {
  return xwalk::GetUserAgent();
}

#if defined(OS_ANDROID)
std::vector<std::unique_ptr<content::NavigationThrottle>>
XWalkContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* navigation_handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
  // We allow intercepting only navigations within main frames. This
  // is used to post onPageStarted. We handle shouldOverrideUrlLoading
  // via a sync IPC.
  if (navigation_handle->IsInMainFrame()) {
    // Use Synchronous mode for the navigation interceptor, since this class
    // doesn't actually call into an arbitrary client, it just posts a task to
    // call onPageStarted. shouldOverrideUrlLoading happens earlier (see
    // ContentBrowserClient::ShouldOverrideUrlLoading).
    throttles.push_back(
        navigation_interception::InterceptNavigationDelegate::CreateThrottleFor(
            navigation_handle, navigation_interception::SynchronyMode::kSync));
    TENTA_LOG(WARNING) << __func__ << " maybe IMPLEMENT";
//    throttles.push_back(std::make_unique<PolicyBlacklistNavigationThrottle>(
//        navigation_handle, AwBrowserContext::FromWebContents(
//                               navigation_handle->GetWebContents())));
  }
  return throttles;
}
#endif // OS_ANDROID

base::Optional<service_manager::Manifest>
XWalkContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  TENTA_LOG(INFO) << __func__ << " manifest=" << name;
  if (name == content::mojom::kBrowserServiceName)
    return GetContentBrowserOverlayManifest();
  if (name == content::mojom::kRendererServiceName)
    return GetContentRendererOverlayManifest();
//  if (name == content::mojom::kUtilityServiceName)
//    return GetAWContentUtilityOverlayManifest();
  return base::nullopt;
}

void XWalkContentBrowserClient::BindInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  TENTA_LOG(INFO) << __func__ << " interface=" << interface_name;
  if (!frame_interfaces_) {
    frame_interfaces_ = std::make_unique<
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>();
    ExposeInterfacesToFrame(frame_interfaces_.get());
  }

  frame_interfaces_->TryBindInterface(interface_name, &interface_pipe,
      render_frame_host);
}

bool XWalkContentBrowserClient::BindAssociatedInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle* handle) {
  TENTA_LOG(INFO) << __func__ << " interface=" << interface_name;
  if (interface_name == autofill::mojom::AutofillDriver::Name_) {
    autofill::ContentAutofillDriverFactory::BindAutofillDriver(
        mojo::PendingAssociatedReceiver<autofill::mojom::AutofillDriver>(
            std::move(*handle)),
        render_frame_host);
    return true;
  }

  return false;
}

void XWalkContentBrowserClient::ExposeInterfacesToFrame(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
    registry) {

}

bool XWalkContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    bool is_navigation,
    bool is_download,
    const url::Origin& request_initiator,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
    network::mojom::TrustedURLLoaderHeaderClientPtrInfo* header_client,
    bool* bypass_redirect_checks) {

  DCHECK(base::FeatureList::IsEnabled(network::features::kNetworkService));
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto proxied_receiver = std::move(*factory_receiver);
  network::mojom::URLLoaderFactoryPtrInfo target_factory_info;
  *factory_receiver = mojo::MakeRequest(&target_factory_info);
  int process_id = is_navigation ? 0 : render_process_id;

  // Android WebView has one non off-the-record browser context.
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&XWalkProxyingURLLoaderFactory::CreateProxy, process_id,
                     std::move(proxied_receiver),
                     std::move(target_factory_info)));
  return true;
}

bool XWalkContentBrowserClient::WillCreateRestrictedCookieManager(
    network::mojom::RestrictedCookieManagerRole role,
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    bool is_service_worker,
    int process_id,
    int routing_id,
    network::mojom::RestrictedCookieManagerRequest* request) {
  network::mojom::RestrictedCookieManagerRequest orig_request =
  std::move(*request);

  network::mojom::RestrictedCookieManagerPtrInfo target_rcm_info;
  *request = mojo::MakeRequest(&target_rcm_info);

  XwalkProxyingRestrictedCookieManager::CreateAndBind(
      std::move(target_rcm_info), is_service_worker, process_id, routing_id,
      std::move(orig_request));

  return false;  // only made a proxy, still need the actual impl to be made.
}

std::unique_ptr<content::LoginDelegate> XWalkContentBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  return std::make_unique<XWalkHttpAuthHandler>(auth_info, web_contents,
                                                first_auth_attempt,
                                                std::move(auth_required_callback));
}

bool XWalkContentBrowserClient::HandleExternalProtocol(const GURL& url,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    network::mojom::URLLoaderFactoryPtr* out_factory) {

  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    auto request = mojo::MakeRequest(out_factory);
    if (content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
      // Manages its own lifetime.
      new XWalkProxyingURLLoaderFactory(
          0 /* process_id */, std::move(request), nullptr,
          true /* intercept_only */);
    } else {
      base::PostTaskWithTraits(
          FROM_HERE, {content::BrowserThread::IO},
          base::BindOnce(
              [](network::mojom::URLLoaderFactoryRequest request) {
                // Manages its own lifetime.
                new XWalkProxyingURLLoaderFactory(
                    0 /* process_id */, std::move(request), nullptr,
                    true /* intercept_only */);
              },
              std::move(request)));
    }
  } else {
    // The AwURLRequestJobFactory implementation should ensure this method never
    // gets called when Network Service is not enabled.
    NOTREACHED();
  }
  return false;
}

void XWalkContentBrowserClient::RegisterNonNetworkSubresourceURLLoaderFactories(
    int render_process_id, int render_frame_id, NonNetworkURLLoaderFactoryMap* factories) {
  LOG(INFO) << "iotto " << __func__;
  WebContents* web_contents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  XWalkSettings* xwalk_settings = XWalkSettings::FromWebContents(web_contents);

  if (xwalk_settings && xwalk_settings->GetAllowFileAccess()) {
    XWalkBrowserContext* browser_context = XWalkBrowserContext::FromWebContents(web_contents);
    auto file_factory = CreateFileURLLoaderFactory(browser_context->GetPath(),
                                                   browser_context->GetSharedCorsOriginAccessList());
    factories->emplace(url::kFileScheme, std::move(file_factory));
  }
}

}  // namespace xwalk
