/*
 * xwalk_content_overlay_manifests.cc
 *
 *  Created on: Feb 17, 2020
 *      Author: iotto
 */

#include "xwalk/runtime/browser/xwalk_content_overlay_manifests.h"

#include "base/no_destructor.h"
#include "components/autofill/content/common/mojom/autofill_agent.mojom.h"
#include "components/autofill/content/common/mojom/autofill_driver.mojom.h"
//#include "components/safe_browsing/common/safe_browsing.mojom.h"
//#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
//#include "components/spellcheck/common/spellcheck.mojom.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"
#include "third_party/blink/public/mojom/input/input_host.mojom.h"
#include "xwalk/runtime/common/js_java_interaction/interfaces.mojom.h"

namespace xwalk {

const service_manager::Manifest& GetContentBrowserOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
//          .ExposeCapability("renderer",
//                            service_manager::Manifest::InterfaceList<
//                                safe_browsing::mojom::SafeBrowsing,
//                                spellcheck::mojom::SpellCheckHost>())
//          .RequireCapability("heap_profiling", "profiling")
//          .RequireCapability("heap_profiling", "heap_profiler")
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "renderer",
              service_manager::Manifest::InterfaceList<
                  autofill::mojom::AutofillDriver,
                  autofill::mojom::PasswordManagerDriver,
                  blink::mojom::TextSuggestionHost,
                  mojom::JsApiHandler>())
          .Build()};
  return *manifest;
}

const service_manager::Manifest& GetContentRendererOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "browser",
              service_manager::Manifest::InterfaceList<
                  autofill::mojom::AutofillAgent,
                  autofill::mojom::PasswordAutofillAgent,
                  autofill::mojom::PasswordGenerationAgent,
//                  safe_browsing::mojom::ThreatReporter,
                  mojom::JsJavaConfigurator, mojom::JsApiHandler>())
          .Build()};
  return *manifest;
}

} /* namespace xwalk */
