// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_XWALK_PLATFORM_NOTIFICATION_SERVICE_H_
#define XWALK_RUNTIME_BROWSER_XWALK_PLATFORM_NOTIFICATION_SERVICE_H_

#include <set>
#include <string>

#include "base/memory/singleton.h"
#include "content/public/browser/platform_notification_service.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom.h"

namespace xwalk {
#if defined(OS_LINUX) && defined(USE_LIBNOTIFY) || defined(OS_WIN)
class XWalkNotificationManager;
#endif

// The platform notification service is the profile-agnostic entry point
// through which Web Notifications can be controlled. Heavily based on
// src/chrome/browser/notifications/platform_notification_service_impl.h.
class XWalkPlatformNotificationService
    : public content::PlatformNotificationService {
 public:
  // Returns the active instance of the service in the browser process. Safe to
  // be called from any thread.
  static XWalkPlatformNotificationService* GetInstance();

//  // content::PlatformNotificationService implementation.
//  blink::mojom::PermissionStatus CheckPermissionOnUIThread(
//      content::BrowserContext* browser_context,
//      const GURL& origin,
//      int render_process_id) override;
//  // content::PlatformNotificationService implementation.
//  blink::mojom::PermissionStatus CheckPermissionOnIOThread(
//      content::ResourceContext* resource_context,
//      const GURL& origin,
//      int render_process_id) override;

  void DisplayNotification(const std::string& notification_id,
                           const GURL& origin, const blink::PlatformNotificationData& notification_data,
                           const blink::NotificationResources& notification_resources) override;

  void DisplayPersistentNotification(const std::string& notification_id,
                                     const GURL& service_worker_origin, const GURL& origin,
                                     const blink::PlatformNotificationData& notification_data,
                                     const blink::NotificationResources& notification_resources) override {
  }
  // Closes the notification identified by |notification_id|. This method must
  // be called on the UI thread.
  void CloseNotification(const std::string& notification_id) override;
  void ClosePersistentNotification(const std::string& notification_id)
      override {
  }
  void GetDisplayedNotifications(DisplayedNotificationsCallback callback) override;
  void ScheduleTrigger(base::Time timestamp) override;
  base::Time ReadNextTriggerTimestamp() override;
  int64_t ReadNextPersistentNotificationId() override;
  void RecordNotificationUkmEvent(const content::NotificationDatabaseData& data) override;

 private:
  friend struct base::DefaultSingletonTraits<XWalkPlatformNotificationService>;

  XWalkPlatformNotificationService();
  ~XWalkPlatformNotificationService() override;

#if defined(OS_LINUX) && defined(USE_LIBNOTIFY)
  std::unique_ptr<XWalkNotificationManager> notification_manager_linux_;
#endif
#if defined(OS_WIN)
  std::unique_ptr<XWalkNotificationManager> notification_manager_win_;
#endif
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_XWALK_PLATFORM_NOTIFICATION_SERVICE_H_
