// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/runtime_download_manager_delegate.h"

#if defined(OS_WIN)
#include <windows.h>
#include <commdlg.h>
#endif

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/shell/common/shell_switches.h"
#include "net/base/filename_util.h"
#include "xwalk/runtime/browser/runtime_platform_util.h"
#include "xwalk/runtime/common/xwalk_paths.h"

#if defined(OS_LINUX)
#include "base/nix/xdg_util.h"
#endif

#if defined(OS_WIN)
#include "ui/base/win/open_file_name_win.h"
#endif

using content::BrowserThread;

namespace xwalk {

RuntimeDownloadManagerDelegate::RuntimeDownloadManagerDelegate()
    : download_manager_(NULL),
      suppress_prompting_(false) {
  // Balanced in Shutdown();
  AddRef();
}

RuntimeDownloadManagerDelegate::~RuntimeDownloadManagerDelegate() {
}

void RuntimeDownloadManagerDelegate::SetDownloadManager(content::DownloadManager* download_manager) {
  download_manager_ = download_manager;
}

void RuntimeDownloadManagerDelegate::Shutdown() {
  Release();
}

bool RuntimeDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LOG(WARNING) << "iotto " << __func__ << " Might need refactoring";
  // This assignment needs to be here because even at the call to
  // SetDownloadManager, the system is not fully initialized.
  if (default_download_path_.empty())
    base::PathService::Get(xwalk::DIR_DOWNLOAD_PATH, &default_download_path_);

  if (!download->GetForcedFilePath().empty()) {
    callback.Run(download->GetForcedFilePath(),
                 download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetForcedFilePath(),
                 download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE);
    return true;
  }

  base::FilePath generated_name = net::GenerateFileName(
      download->GetURL(),
      download->GetContentDisposition(),
      std::string(),
      download->GetSuggestedFilename(),
      download->GetMimeType(),
      "download");

  base::PostTaskWithTraits(FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(
          &RuntimeDownloadManagerDelegate::GenerateFilename,
          this, download->GetId(), callback, generated_name,
          default_download_path_));
  return true;
//  // Note this cancel is independent of the URLRequest cancel in
//  // AwResourceDispatcherHostDelegate::DownloadStarting. The request
//  // could have already finished by the time DownloadStarting is called.
//  callback.Run(base::FilePath() /* Empty file path for cancel */,
//               download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
//               download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, base::FilePath(),
//               download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
//  return true;
}

bool RuntimeDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* item,
      const content::DownloadOpenDelayedCallback& callback) {
  return true;
}

void RuntimeDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = download::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

void RuntimeDownloadManagerDelegate::GenerateFilename(
    uint32_t download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& generated_name,
    const base::FilePath& suggested_directory) {
  if (!base::CreateDirectory(suggested_directory)) {
    LOG(ERROR) << "Failed to create directory: "
               << suggested_directory.value();
    return;
  }

  base::FilePath suggested_path(suggested_directory.Append(generated_name));
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(
          &RuntimeDownloadManagerDelegate::OnDownloadPathGenerated,
          this, download_id, callback, suggested_path));
}

void RuntimeDownloadManagerDelegate::OnDownloadPathGenerated(
    uint32_t download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& suggested_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (suppress_prompting_) {
    // Testing exit.
    callback.Run(suggested_path,
                 download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 suggested_path.AddExtension(FILE_PATH_LITERAL(".crdownload")),
                 download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE);
    return;
  }

  ChooseDownloadPath(download_id, callback, suggested_path);
}

void RuntimeDownloadManagerDelegate::ChooseDownloadPath(
    uint32_t download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& suggested_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  download::DownloadItem* item = download_manager_->GetDownload(download_id);
  if (!item || (item->GetState() != download::DownloadItem::IN_PROGRESS))
    return;

#if defined(OS_LINUX) || defined(OS_WIN)
  base::FilePath result;
  content::WebContents* contents = item->GetWebContents();
  Runtime* runtime = static_cast<Runtime*>(contents->GetDelegate());
  if (!runtime->AddDownloadItem(item, callback, suggested_path)) {
    const base::FilePath empty;
    callback.Run(empty,
        content::DownloadItem::TARGET_DISPOSITION_PROMPT,
        content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
        suggested_path);
  }
#else
  NOTIMPLEMENTED();
#endif
}

void RuntimeDownloadManagerDelegate::SetDownloadBehaviorForTesting(
    const base::FilePath& default_download_path) {
  default_download_path_ = default_download_path;
  suppress_prompting_ = true;
}

}  // namespace xwalk
