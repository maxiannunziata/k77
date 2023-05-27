// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/xwalk_icon_helper.h"

#include "base/bind.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/favicon_url.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"
#include "meta_logging.h"

using content::BrowserThread;
using content::WebContents;

namespace xwalk {

XWalkIconHelper::XWalkIconHelper(WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      listener_(NULL),
      _this_weak(this) {
}

XWalkIconHelper::~XWalkIconHelper() {
  _this_weak.InvalidateWeakPtrs();
}

void XWalkIconHelper::SetListener(Listener* listener) {
  listener_ = listener;
}

void XWalkIconHelper::DownloadIcon(const GURL& icon_url) {
  TENTA_LOG_NET(INFO) << __func__ << " url=" << icon_url;
  web_contents()->DownloadImage(icon_url, true, 0, false,
      base::Bind(&XWalkIconHelper::DownloadFaviconCallback,
                 _this_weak.GetWeakPtr()));
}

void XWalkIconHelper::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& candidates) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  content::FaviconURL largest_icon;
  gfx::Size max_size;

  // TODO(iotto): Remove logs if confirmed all ok
  for (std::vector<content::FaviconURL>::const_iterator i = candidates.begin();
       i != candidates.end(); ++i) {
    if (!i->icon_url.is_valid())
      continue;
    TENTA_LOG_NET(INFO) << __func__ << " type=" << (int)i->icon_type << " url=" << i->icon_url;
    switch (i->icon_type) {
      case content::FaviconURL::IconType::kFavicon: {
        if ( !i->icon_sizes.empty()) {
          TENTA_LOG_NET(INFO) << __func__ << " type_is_favicon url=" << i->icon_url << " icon_sizes="
                    << i->icon_sizes.size();
          for (auto& ico_size : i->icon_sizes) {
            TENTA_LOG_NET(INFO) << __func__ << " msw=" << max_size.width() << " msh=" << max_size.height() << " icw="
                      << ico_size.width() << " ich=" << ico_size.height();

            if ((max_size.width() < ico_size.width()) || (max_size.height() < ico_size.height())) {
              max_size = ico_size;
              largest_icon = *i;
              TENTA_LOG_NET(INFO) << __func__ << " found largestIcon=" << largest_icon.icon_url;
            }
          }

        }

        if (!largest_icon.icon_url.is_valid()) {
          largest_icon = *i;
          TENTA_LOG_NET(INFO) << __func__ << " largestIconUpdated url=" << largest_icon.icon_url;
        }

        break;
      }
      case content::FaviconURL::IconType::kTouchIcon:
        break;
      case content::FaviconURL::IconType::kTouchPrecomposedIcon:
        break;
      case content::FaviconURL::IconType::kInvalid:
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  if (largest_icon.icon_url.is_valid()) {
    if ( listener_) {
      listener_->OnIconAvailable(largest_icon.icon_url);
    }
  }
//    LOG(INFO) << "iotto " << __func__ << " type_is_favicon will_start_load url=" << largest_icon.icon_url;
////        if (listener_) listener_->OnIconAvailable(i->icon_url);
//    // TODO(iotto) : Ask Clinet if favicons should be downloaded
//    web_contents()->DownloadImage(largest_icon.icon_url, true,  // Is a favicon
//                                  0,  // No maximum size
//                                  false,  // Normal cache policy
//                                  base::Bind(&XWalkIconHelper::DownloadFaviconCallback, _this_weak.GetWeakPtr()));
//  }

}

void XWalkIconHelper::DidFinishNavigation(content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;
}

void XWalkIconHelper::DownloadFaviconCallback(
    int id,
    int http_status_code,
    const GURL& image_url,
    const std::vector<SkBitmap>& bitmaps,
    const std::vector<gfx::Size>& original_bitmap_sizes) {
  TENTA_LOG_NET(INFO) << __func__ << " http_status_code=" << http_status_code << " image_url=" << image_url
            << " bitmaps_size=" << bitmaps.size() << " original_bitmap_sizes=" << original_bitmap_sizes.size();
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (http_status_code == 404 || bitmaps.size() == 0) return;

  // find largest bitmap
  gfx::Size max_size;
  int bitmap_pos = 0;

  for ( int i = 0; i < (int)original_bitmap_sizes.size(); ++i) {
    const gfx::Size& bitmap_size = original_bitmap_sizes[i];

    if ( (max_size.width() < bitmap_size.width()) || (max_size.height() < bitmap_size.height())) {
      max_size = bitmap_size;
      bitmap_pos = i;
    }
  }
  if (listener_) listener_->OnReceivedIcon(image_url, bitmaps[bitmap_pos]);
}

}  // namespace xwalk
