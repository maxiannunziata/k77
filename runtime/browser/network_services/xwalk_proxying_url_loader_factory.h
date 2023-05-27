/*
 * xwalk_proxying_url_loader_factory.h
 *
 *  Created on: Feb 15, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_PROXYING_URL_LOADER_FACTORY_H_
#define XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_PROXYING_URL_LOADER_FACTORY_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace xwalk {
// see android_webview/browser/network_service/aw_proxying_url_loader_factory.h
// URL Loader Factory for Android WebView. This is the entry point for handling
// Android WebView callbacks (i.e. error, interception and other callbacks) and
// loading of android specific schemes and overridden responses.
//
// This class contains centralized logic for:
//  - request interception and blocking,
//  - setting load flags and headers,
//  - loading requests depending on the scheme (e.g. different delegates are
//    used for loading android assets/resources as compared to overridden
//    responses).
//  - handling errors (e.g. no input stream, redirect or safebrowsing related
//    errors).
//
// In particular handles the following Android WebView callbacks:
//  - shouldInterceptRequest
//  - onReceivedError
//  - onReceivedHttpError
//  - onReceivedLoginRequest
//
// Threading:
//  Currently the factory and the associated loader assume they live on the IO
//  thread. This is also required by the shouldInterceptRequest callback (which
//  should be called on a non-UI thread). The other callbacks (i.e.
//  onReceivedError, onReceivedHttpError and onReceivedLoginRequest) are posted
//  on the UI thread.
//
class XWalkProxyingURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  // Create a factory that will create specialized URLLoaders for Android
  // WebView. If |intercept_only| parameter is true the loader created by
  // this factory will only execute the intercept callback
  // (shouldInterceptRequest), it will not propagate the request to the
  // target factory.
  XWalkProxyingURLLoaderFactory(int process_id, network::mojom::URLLoaderFactoryRequest loader_request,
                                network::mojom::URLLoaderFactoryPtrInfo target_factory_info, bool intercept_only);

  ~XWalkProxyingURLLoaderFactory() override;

  // static
  static void CreateProxy(int process_id, network::mojom::URLLoaderFactoryRequest loader,
                          network::mojom::URLLoaderFactoryPtrInfo target_factory_info);

  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader, int32_t routing_id, int32_t request_id,
                            uint32_t options, const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) override;

  void Clone(network::mojom::URLLoaderFactoryRequest loader_request) override;

 private:
  void OnTargetFactoryError();
  void OnProxyBindingError();

  const int process_id_;
  mojo::BindingSet<network::mojom::URLLoaderFactory> proxy_bindings_;
  network::mojom::URLLoaderFactoryPtr target_factory_;

  // When true the loader resulting from this factory will only execute
  // intercept callback (shouldInterceptRequest). If that returns without
  // a response, the loader will abort loading.
  bool intercept_only_;

  base::WeakPtrFactory<XWalkProxyingURLLoaderFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(XWalkProxyingURLLoaderFactory);
};

} /* namespace xwalk */

#endif /* XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_PROXYING_URL_LOADER_FACTORY_H_ */
