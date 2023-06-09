// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/runtime_file_select_helper.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "xwalk/runtime/browser/runtime_platform_util.h"
#include "xwalk/runtime/browser/runtime_select_file_policy.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/file_chooser_file_info.h"
#include "content/public/common/file_chooser_params.h"
#include "grit/xwalk_resources.h"
#include "net/base/mime_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/selected_file_info.h"

using content::BrowserThread;
using content::FileChooserParams;
using content::RenderViewHost;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::WebContents;

namespace {

// There is only one file-selection happening at any given time,
// so we allocate an enumeration ID for that purpose.  All IDs from
// the renderer must start at 0 and increase.
const int kFileSelectEnumerationId = -1;

void NotifyRenderFrameHost(content::RenderFrameHost* render_frame_host,
                          const std::vector<ui::SelectedFileInfo>& files,
                          FileChooserParams::Mode dialog_mode) {
  std::vector<content::FileChooserFileInfo> chooser_files;
  for (const auto& file : files) {
    content::FileChooserFileInfo chooser_file;
    chooser_file.file_path = file.local_path;
    chooser_file.display_name = file.display_name;
    chooser_files.push_back(chooser_file);
  }
  render_frame_host->FilesSelectedInChooser(chooser_files, dialog_mode);
}

// Converts a list of FilePaths to a list of ui::SelectedFileInfo.
std::vector<ui::SelectedFileInfo> FilePathListToSelectedFileInfoList(
    const std::vector<base::FilePath>& paths) {
  std::vector<ui::SelectedFileInfo> selected_files;
  for (size_t i = 0; i < paths.size(); ++i) {
    selected_files.push_back(
        ui::SelectedFileInfo(paths[i], paths[i]));
  }
  return selected_files;
}

}  // namespace

struct RuntimeFileSelectHelper::ActiveDirectoryEnumeration {
  ActiveDirectoryEnumeration() : render_view_host_(nullptr) {}

  std::unique_ptr<DirectoryListerDispatchDelegate> delegate_;
  std::unique_ptr<net::DirectoryLister> lister_;
  RenderViewHost* render_view_host_;
  std::vector<base::FilePath> results_;
};

RuntimeFileSelectHelper::RuntimeFileSelectHelper()
    : render_frame_host_(nullptr),
      web_contents_(nullptr),
      select_file_dialog_(),
      select_file_types_(),
      dialog_type_(ui::SelectFileDialog::SELECT_OPEN_FILE),
      dialog_mode_(FileChooserParams::Open) {
}

RuntimeFileSelectHelper::~RuntimeFileSelectHelper() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();

  // Stop any pending directory enumeration, prevent a callback, and free
  // allocated memory.
  std::map<int, ActiveDirectoryEnumeration*>::iterator iter;
  for (iter = directory_enumerations_.begin();
       iter != directory_enumerations_.end();
       ++iter) {
    iter->second->lister_.reset();
    delete iter->second;
  }
}

void RuntimeFileSelectHelper::DirectoryListerDispatchDelegate::OnListFile(
    const net::DirectoryLister::DirectoryListerData& data) {
  parent_->OnListFile(id_, data);
}

void RuntimeFileSelectHelper::DirectoryListerDispatchDelegate::OnListDone(
    int error) {
  parent_->OnListDone(id_, error);
}

void RuntimeFileSelectHelper::FileSelected(const base::FilePath& path,
                                           int index, void* params) {
  FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), index, params);
}

void RuntimeFileSelectHelper::FileSelectedWithExtraInfo(
    const ui::SelectedFileInfo& file,
    int index,
    void* params) {
  if (!render_frame_host_)
    return;

  // TODO(wang16): Save last select directory here

  const base::FilePath& path = file.local_path;
  if (dialog_type_ == ui::SelectFileDialog::SELECT_FOLDER) {
    StartNewEnumeration(path,
                        kFileSelectEnumerationId,
                        render_frame_host_->GetRenderViewHost());
    return;
  }

  std::vector<ui::SelectedFileInfo> files;
  files.push_back(file);
  NotifyRenderFrameHost(render_frame_host_, files, dialog_mode_);

  // No members should be accessed from here on.
  RunFileChooserEnd();
}

void RuntimeFileSelectHelper::MultiFilesSelected(
    const std::vector<base::FilePath>& files,
    void* params) {
  std::vector<ui::SelectedFileInfo> selected_files =
      FilePathListToSelectedFileInfoList(files);

  MultiFilesSelectedWithExtraInfo(selected_files, params);
}

void RuntimeFileSelectHelper::MultiFilesSelectedWithExtraInfo(
    const std::vector<ui::SelectedFileInfo>& files,
    void* params) {

  // TODO(wang16): Save last select directory here

  if (!render_frame_host_)
    return;

  NotifyRenderFrameHost(render_frame_host_, files, dialog_mode_);

  // No members should be accessed from here on.
  RunFileChooserEnd();
}

void RuntimeFileSelectHelper::FileSelectionCanceled(void* params) {
  if (!render_frame_host_)
    return;

  // If the user cancels choosing a file to upload we pass back an
  // empty vector.
  NotifyRenderFrameHost(
      render_frame_host_, std::vector<ui::SelectedFileInfo>(),
      dialog_mode_);

  // No members should be accessed from here on.
  RunFileChooserEnd();
}

void RuntimeFileSelectHelper::StartNewEnumeration(
    const base::FilePath& path,
    int request_id,
    RenderViewHost* render_view_host) {
  std::unique_ptr<ActiveDirectoryEnumeration> entry(new ActiveDirectoryEnumeration);
  entry->render_view_host_ = render_view_host;
  entry->delegate_.reset(new DirectoryListerDispatchDelegate(this, request_id));
  entry->lister_.reset(new net::DirectoryLister(path,
                               net::DirectoryLister::NO_SORT_RECURSIVE,
                               entry->delegate_.get()));
//  if (!entry->lister_->Start()) {
    entry->lister_->Start();

    if (request_id == kFileSelectEnumerationId)
      FileSelectionCanceled(NULL);
    else
      render_view_host->DirectoryEnumerationFinished(request_id,
                                                     entry->results_);
//  } else {
//    directory_enumerations_[request_id] = entry.release();
//  }
}

void RuntimeFileSelectHelper::OnListFile(
    int id,
    const net::DirectoryLister::DirectoryListerData& data) {
  ActiveDirectoryEnumeration* entry = directory_enumerations_[id];

  // Directory upload returns directories via a "." file, so that
  // empty directories are included.  This util call just checks
  // the flags in the structure; there's no file I/O going on.
  if (data.info.IsDirectory())
    entry->results_.push_back(data.path.Append(FILE_PATH_LITERAL(".")));
  else
    entry->results_.push_back(data.path);
}

void RuntimeFileSelectHelper::OnListDone(int id, int error) {
  // This entry needs to be cleaned up when this function is done.
  std::unique_ptr<ActiveDirectoryEnumeration> entry(directory_enumerations_[id]);
  directory_enumerations_.erase(id);
  if (!entry->render_view_host_)
    return;
  if (error) {
    FileSelectionCanceled(NULL);
    return;
  }

  std::vector<ui::SelectedFileInfo> selected_files =
      FilePathListToSelectedFileInfoList(entry->results_);

  if (id == kFileSelectEnumerationId)
    NotifyRenderFrameHost(
        render_frame_host_, selected_files, dialog_mode_);
  else
    entry->render_view_host_->DirectoryEnumerationFinished(id, entry->results_);

  EnumerateDirectoryEnd();
}

std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
RuntimeFileSelectHelper::GetFileTypesFromAcceptType(
    const std::vector<base::string16>& accept_types) {
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> base_file_type(
      new ui::SelectFileDialog::FileTypeInfo());
  base_file_type->allowed_paths = ui::SelectFileDialog::FileTypeInfo::ANY_PATH;
  if (accept_types.empty())
    return base_file_type;

  // Create FileTypeInfo and pre-allocate for the first extension list.
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> file_type(
      new ui::SelectFileDialog::FileTypeInfo(*base_file_type));
  file_type->include_all_files = true;
  file_type->extensions.resize(1);
  std::vector<base::FilePath::StringType>* extensions =
      &file_type->extensions.back();

  // Find the corresponding extensions.
  int valid_type_count = 0;
  int description_id = 0;
  for (size_t i = 0; i < accept_types.size(); ++i) {
    std::string ascii_type = base::UTF16ToASCII(accept_types[i]);
    if (!IsAcceptTypeValid(ascii_type))
      continue;

    size_t old_extension_size = extensions->size();
    if (ascii_type[0] == '.') {
      // If the type starts with a period it is assumed to be a file extension
      // so we just have to add it to the list.
      base::FilePath::StringType ext(ascii_type.begin(), ascii_type.end());
      extensions->push_back(ext.substr(1));
    } else {
      if (ascii_type == "image/*")
        description_id = IDS_IMAGE_FILES;
      else if (ascii_type == "audio/*")
        description_id = IDS_AUDIO_FILES;
      else if (ascii_type == "video/*")
        description_id = IDS_VIDEO_FILES;

      net::GetExtensionsForMimeType(ascii_type, extensions);
    }

    if (extensions->size() > old_extension_size)
      valid_type_count++;
  }

  // If no valid extension is added, bail out.
  if (valid_type_count == 0)
    return base_file_type;

  // Use a generic description "Custom Files" if either of the following is
  // true:
  // 1) There're multiple types specified, like "audio/*,video/*"
  // 2) There're multiple extensions for a MIME type without parameter, like
  //    "ehtml,shtml,htm,html" for "text/html". On Windows, the select file
  //    dialog uses the first extension in the list to form the description,
  //    like "EHTML Files". This is not what we want.
  if (valid_type_count > 1 ||
      (valid_type_count == 1 && description_id == 0 && extensions->size() > 1))
    description_id = IDS_CUSTOM_FILES;

  if (description_id) {
    file_type->extension_description_overrides.push_back(
        l10n_util::GetStringUTF16(description_id));
  }

  return file_type;
}

// static
void RuntimeFileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const FileChooserParams& params) {
  // RuntimeFileSelectHelper will keep itself alive until it sends the
  // result message.
  scoped_refptr<RuntimeFileSelectHelper> file_select_helper(
      new RuntimeFileSelectHelper());
  file_select_helper->RunFileChooser(
      render_frame_host,
      WebContents::FromRenderFrameHost(render_frame_host),
      params);
}

// static
void RuntimeFileSelectHelper::EnumerateDirectory(content::WebContents* tab,
                                                 int request_id,
                                                 const base::FilePath& path) {
  // RuntimeFileSelectHelper will keep itself alive until it sends the
  // result message.
  scoped_refptr<RuntimeFileSelectHelper> file_select_helper(
      new RuntimeFileSelectHelper());
  file_select_helper->EnumerateDirectory(
      request_id, tab->GetRenderViewHost(), path);
}

void RuntimeFileSelectHelper::RunFileChooser(content::RenderFrameHost* render_frame_host,
                          content::WebContents* web_contents,
                          const blink::mojom::::FileChooserParams& params) {
  DCHECK(!render_frame_host_);
  DCHECK(!web_contents_);
  render_frame_host_ = render_frame_host;
  web_contents_ = web_contents;
  notification_registrar_.RemoveAll();
  notification_registrar_.Add(
      this, content::NOTIFICATION_RENDER_WIDGET_HOST_DESTROYED,
      content::Source<RenderWidgetHost>(
          render_frame_host_->GetRenderViewHost()->GetWidget()));
  notification_registrar_.Add(
      this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
      content::Source<WebContents>(web_contents_));

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&RuntimeFileSelectHelper::RunFileChooserOnFileThread,
                 this,
                 params));

  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the file dialog.
  // At that point, we must call RunFileChooserEnd().
  AddRef();
}

void RuntimeFileSelectHelper::RunFileChooserOnFileThread(
    const FileChooserParams& params) {
  select_file_types_ = GetFileTypesFromAcceptType(params.accept_types);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&RuntimeFileSelectHelper::RunFileChooserOnUIThread,
                 this,
                 params));
}

void RuntimeFileSelectHelper::RunFileChooserOnUIThread(
    const FileChooserParams& params) {
  dialog_mode_ = params.mode;
  if (!render_frame_host_ || !web_contents_) {
    // If the renderer was destroyed before we started, just cancel the
    // operation.
    RunFileChooserEnd();
    return;
  }

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, base::WrapUnique(new RuntimeSelectFilePolicy()));

  switch (params.mode) {
    case FileChooserParams::Open:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      break;
    case FileChooserParams::OpenMultiple:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE;
      break;
    case FileChooserParams::UploadFolder:
      dialog_type_ = ui::SelectFileDialog::SELECT_UPLOAD_FOLDER;
      break;
    case FileChooserParams::Save:
      dialog_type_ = ui::SelectFileDialog::SELECT_SAVEAS_FILE;
      break;
    default:
      // Prevent warning.
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      NOTREACHED();
  }

  base::FilePath default_file_name = params.default_file_name.IsAbsolute() ?
      params.default_file_name :
      // TODO(wang16): Load last select directory here
      base::FilePath();

  gfx::NativeWindow owning_window =
      platform_util::GetTopLevel(web_contents_->GetNativeView());

#if defined(OS_ANDROID)
  // Android needs the original MIME types and an additional capture value.
  std::pair<std::vector<base::string16>, bool> accept_types =
      std::make_pair(params.accept_types, params.capture);
#endif

  select_file_dialog_->SelectFile(
      dialog_type_,
      params.title,
      default_file_name,
      select_file_types_.get(),
      select_file_types_.get() && !select_file_types_->extensions.empty()
          ? 1
          : 0,  // 1-based index of default extension to show.
      base::FilePath::StringType(),
      owning_window,
#if defined(OS_ANDROID)
      &accept_types);
#else
      web_contents_);
#endif

  select_file_types_.reset();
}

// This method is called when we receive the last callback from the file
// chooser dialog. Perform any cleanup and release the reference we added
// in RunFileChooser().
void RuntimeFileSelectHelper::RunFileChooserEnd() {
  render_frame_host_ = NULL;
  web_contents_ = NULL;
  Release();
}

void RuntimeFileSelectHelper::EnumerateDirectory(
    int request_id,
    RenderViewHost* render_view_host,
    const base::FilePath& path) {

  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the enumeration
  // code. At that point, we must call EnumerateDirectoryEnd().
  AddRef();
  StartNewEnumeration(path, request_id, render_view_host);
}

// This method is called when we receive the last callback from the enumeration
// code. Perform any cleanup and release the reference we added in
// EnumerateDirectory().
void RuntimeFileSelectHelper::EnumerateDirectoryEnd() {
  Release();
}

void RuntimeFileSelectHelper::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_RENDER_WIDGET_HOST_DESTROYED: {
      DCHECK(content::Source<RenderWidgetHost>(source).ptr() ==
            render_frame_host_->GetRenderViewHost()->GetWidget());
      render_frame_host_ = NULL;
      break;
    }

    case content::NOTIFICATION_WEB_CONTENTS_DESTROYED: {
      DCHECK(content::Source<WebContents>(source).ptr() == web_contents_);
      web_contents_ = NULL;
      break;
    }

    default:
      NOTREACHED();
  }
}

// static
bool RuntimeFileSelectHelper::IsAcceptTypeValid(
    const std::string& accept_type) {
  // TODO(raymes): This only does some basic checks, extend to test more cases.
  // A 1 character accept type will always be invalid (either a "." in the case
  // of an extension or a "/" in the case of a MIME type).
  std::string unused;
  if (accept_type.length() <= 1 ||
      base::ToLowerASCII(accept_type) != accept_type ||
      base::TrimWhitespaceASCII(
        accept_type,
        base::TRIM_ALL, &unused) != base::TRIM_NONE) {
    return false;
  }
  return true;
}
