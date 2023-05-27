/*
 * xwalk_content_overlay_manifests.h
 *
 *  Created on: Feb 17, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_XWALK_CONTENT_OVERLAY_MANIFESTS_H_
#define XWALK_RUNTIME_BROWSER_XWALK_CONTENT_OVERLAY_MANIFESTS_H_

#include "services/service_manager/public/cpp/manifest.h"

namespace xwalk {

// Returns the manifest Android WebView amends to Content's content_browser
// service manifest. This allows WebView to extend the capabilities exposed
// and/or required by content_browser service instances, as well as declaring
// any additional in- and out-of-process per-profile packaged services.
const service_manager::Manifest& GetContentBrowserOverlayManifest();

// Returns the manifest Android WebView amends to Content's content_renderer
// service manifest. This allows WebView to extend the capabilities exposed
// and/or required by content_renderer service instances.
const service_manager::Manifest& GetContentRendererOverlayManifest();

} /* namespace xwalk */

#endif /* XWALK_RUNTIME_BROWSER_XWALK_CONTENT_OVERLAY_MANIFESTS_H_ */
