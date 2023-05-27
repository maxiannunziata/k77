// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/app/android/xwalk_main_delegate_android.h"

#include <memory>
#include <string>

#include "base/android/apk_assets.h"
#include "base/android/locale_utils.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/base_paths_android.h"
#include "base/path_service.h"
#include "base/files/file.h"
#include "base/posix/global_descriptors.h"
#include "cc/base/switches.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/common/content_switches.h"
#include "components/autofill/core/common/autofill_features.h"
#include "gpu/config/gpu_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "xwalk/runtime/common/android/xwalk_descriptors.h"
#include "xwalk/runtime/common/android/xwalk_globals_android.h"
#include "xwalk/runtime/common/xwalk_content_client.h"
#include "xwalk/runtime/common/xwalk_paths.h"
#include "xwalk/runtime/common/xwalk_resource_delegate.h"
#include "xwalk/runtime/gpu/xwalk_content_gpu_client.h"

#include "android_webview/browser/scoped_add_feature_flags.h"
#include "components/viz/common/features.h"
#include "content/public/common/content_features.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/config/gpu_finch_features.h"
#include "media/base/media_switches.h"
#include "third_party/blink/public/common/features.h"
#include "ui/display/display_switches.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_switches.h"

namespace xwalk {

namespace {
void InitIcuAndResourceBundleBrowserSide() {
  // TODO(iotto): load manually instead of LOAD_COMMON_RESOURCES
  // rename in resources Build.gn chrome_100 to xwalk_100
//  void ResourceBundle::LoadCommonResources() {
//    base::FilePath disk_path;
//    base::PathService::Get(ui::DIR_RESOURCE_PAKS_ANDROID, &disk_path);
//    disk_path = disk_path.AppendASCII("chrome_100_percent.pak");
//    bool success =
//        LoadFromApkOrFile("assets/chrome_100_percent.pak", &disk_path,
//                          &g_chrome_100_percent_fd, &g_chrome_100_percent_region);
//    DCHECK(success);
//
//    AddDataPackFromFileRegion(base::File(g_chrome_100_percent_fd),
//                              g_chrome_100_percent_region, SCALE_FACTOR_100P);
//  }
  ui::SetLocalePaksStoredInApk(true);
  std::string default_locale(base::android::GetDefaultLocaleString());
  LOG(INFO) << "iotto " << __func__ << " defaultLocale=" << default_locale;
  std::string locale = ui::ResourceBundle::InitSharedInstanceWithLocale(
      default_locale, NULL,
      ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  if (locale.empty()) {
    LOG(WARNING) << __func__ << " Failed to load locale .pak from apk.";
  }
  base::i18n::SetICUDefaultLocale(locale);

  // Try to directly mmap the resources.pak from the apk. Fall back to load
  // from file, using PATH_SERVICE, otherwise.
  base::FilePath pak_file_path;
  base::PathService::Get(ui::DIR_RESOURCE_PAKS_ANDROID, &pak_file_path);
  pak_file_path = pak_file_path.AppendASCII("xwalk.pak");
  ui::LoadMainAndroidPackFile("assets/xwalk.pak", pak_file_path);
}

void InitResourceBundleRendererSide() {
  LOG(INFO) << "iotto " << __func__;
  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->Get(kXWalkLocalePakDescriptor);
  LOG(INFO) << "iotto " << __func__ << " packfd=" << pak_fd;
  base::MemoryMappedFile::Region pak_region = global_descriptors->GetRegion(kXWalkLocalePakDescriptor);
  ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(base::File(pak_fd), pak_region);

  std::pair<int, ui::ScaleFactor> extra_paks[] = { { kXWalkMainPakDescriptor, ui::SCALE_FACTOR_NONE }, {
      kXWalk100PakDescriptor, ui::SCALE_FACTOR_100P } };

  for (const auto& pak_info : extra_paks) {
    pak_fd = global_descriptors->Get(pak_info.first);
    pak_region = global_descriptors->GetRegion(pak_info.first);
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(base::File(pak_fd), pak_region, pak_info.second);
  }

  LOG(INFO) << "iotto " << __func__ << " done";

}
}
// TODO(iotto): Continue
//namespace {
//gpu::SyncPointManager* GetSyncPointManager() {
//  DCHECK(DeferredGpuCommandService::GetInstance());
//  return DeferredGpuCommandService::GetInstance()->sync_point_manager();
//}
//gpu::SharedImageManager* GetSharedImageManager() {
//  DCHECK(DeferredGpuCommandService::GetInstance());
//  const bool enable_shared_image =
//      base::CommandLine::ForCurrentProcess()->HasSwitch(
//          switches::kWebViewEnableSharedImage);
//  return enable_shared_image
//             ? DeferredGpuCommandService::GetInstance()->shared_image_manager()
//             : nullptr;
//}
//}  // namespace

XWalkMainDelegateAndroid::XWalkMainDelegateAndroid() {
}

XWalkMainDelegateAndroid::~XWalkMainDelegateAndroid() {
}

bool XWalkMainDelegateAndroid::BasicStartupComplete(int* exit_code) {

  content_client_.reset(new XWalkContentClient);
  content::SetContentClient(content_client_.get());

  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  // Not yet supported in single-process mode.
  cl->AppendSwitch(switches::kDisableSharedWorkers);

  // File system API not supported (requires some new API; internal bug 6930981)
  cl->AppendSwitch(switches::kDisableFileSystem);

  // Web Notification API and the Push API are not supported (crbug.com/434712)
  cl->AppendSwitch(switches::kDisableNotifications);

  // WebRTC hardware decoding is not supported, internal bug 15075307
  cl->AppendSwitch(switches::kDisableWebRtcHWDecoding);

  // Check damage in OnBeginFrame to prevent unnecessary draws.
  cl->AppendSwitch(cc::switches::kCheckDamageEarly);

  // This is needed for sharing textures across the different GL threads.
  cl->AppendSwitch(switches::kEnableThreadedTextureMailboxes);

  // WebView does not yet support screen orientation locking.
  cl->AppendSwitch(switches::kDisableScreenOrientationLock);

  // WebView does not currently support Web Speech Synthesis API,
  // but it does support Web Speech Recognition API (crbug.com/487255).
  cl->AppendSwitch(switches::kDisableSpeechSynthesisAPI);

  // WebView does not currently support the Permissions API (crbug.com/490120)
  cl->AppendSwitch(switches::kDisablePermissionsAPI);

  // WebView does not (yet) save Chromium data during shutdown, so add setting
  // for Chrome to aggressively persist DOM Storage to minimize data loss.
  // http://crbug.com/479767
  cl->AppendSwitch(switches::kEnableAggressiveDOMStorageFlushing);

  // Webview does not currently support the Presentation API, see
  // https://crbug.com/521319
  cl->AppendSwitch(switches::kDisablePresentationAPI);

  // WebView doesn't support Remote Playback API for the same reason as the
  // Presentation API, see https://crbug.com/521319.
  cl->AppendSwitch(switches::kDisableRemotePlaybackAPI);

  // WebView does not support MediaSession API since there's no UI for media
  // metadata and controls.
  cl->AppendSwitch(switches::kDisableMediaSessionAPI);

  {
      android_webview::ScopedAddFeatureFlags features(cl);
//o
////  #if BUILDFLAG(ENABLE_SPELLCHECK)
////      features.EnableIfNotSet(spellcheck::kAndroidSpellCheckerNonLowEnd);
////  #endif  // ENABLE_SPELLCHECK
//
      features.EnableIfNotSet(
          autofill::features::kAutofillSkipComparingInferredLabels);
////
////      if (cl->HasSwitch(switches::kWebViewLogJsConsoleMessages)) {
////        features.EnableIfNotSet(::features::kLogJsConsoleMessages);
////      }
////
////      features.DisableIfNotSet(::features::kWebPayments);
////
////      // WebView does not and should not support WebAuthN.
////      features.DisableIfNotSet(::features::kWebAuth);
//
//      // WebView isn't compatible with OOP-D.
////      features.DisableIfNotSet(::features::kVizDisplayCompositor);

      // WebView does not support AndroidOverlay yet for video overlays.
//      features.DisableIfNotSet(media::kUseAndroidOverlay);

      // WebView doesn't support embedding CompositorFrameSinks which is needed
      // for UseSurfaceLayerForVideo feature. https://crbug.com/853832
      features.EnableIfNotSet(media::kDisableSurfaceLayerForVideo);

      // WebView does not support EME persistent license yet, because it's not
      // clear on how user can remove persistent media licenses from UI.
      features.DisableIfNotSet(media::kMediaDrmPersistentLicense);

      features.DisableIfNotSet(
          autofill::features::kAutofillRestrictUnownedFieldsToFormlessCheckout);

      features.DisableIfNotSet(::features::kBackgroundFetch);

      features.DisableIfNotSet(::features::kAndroidSurfaceControl);

      // TODO(https://crbug.com/963653): SmsReceiver is not yet supported on
      // WebView.
      features.DisableIfNotSet(::features::kSmsReceiver);

      // WebView relies on tweaking the size heuristic to dynamically enable
      // or disable accelerated cavnas 2d.
      features.DisableIfNotSet(blink::features::kAlwaysAccelerateCanvas);
    }

  RegisterPathProvider();
  return false;
}

void XWalkMainDelegateAndroid::PreSandboxStartup() {
  LOG(INFO) << "iotto " << __func__;
#if defined(ARCH_CPU_ARM_FAMILY)
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info for ARM platform.
  base::CPU cpu_info;
#endif
  InitResourceBundle();
}

int XWalkMainDelegateAndroid::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (process_type.empty()) {
    browser_runner_ = content::BrowserMainRunner::Create();
    int exit_code = browser_runner_->Initialize(main_function_params);
    DCHECK_LT(exit_code, 0);

    return 0;
  }
  return -1;
}

// This function is called only on the browser process.
void XWalkMainDelegateAndroid::PostEarlyInitialization(bool is_running_tests) {
  InitIcuAndResourceBundleBrowserSide();
}

void XWalkMainDelegateAndroid::InitResourceBundle() {
//  AW style
  const base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();

  std::string process_type = command_line.GetSwitchValueASCII(switches::kProcessType);
  const bool is_browser_process = process_type.empty();
  if (!is_browser_process) {
    base::i18n::SetICUDefaultLocale(command_line.GetSwitchValueASCII(switches::kLang));
  }

  if (process_type == switches::kRendererProcess) {
    InitResourceBundleRendererSide();
  }
}

content::ContentGpuClient* XWalkMainDelegateAndroid::CreateContentGpuClient() {
  content_gpu_client_ = std::make_unique<XWalkContentGpuClient>();
//  content_gpu_client_ = std::make_unique<XWalkContentGpuClient>(
//      base::BindRepeating(&GetSyncPointManager),
//      base::BindRepeating(&GetSharedImageManager));
  return content_gpu_client_.get();
}

}  // namespace xwalk
