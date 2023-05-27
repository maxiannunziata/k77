/*
 * xwalk_render_frame_ext.cpp
 *
 *  Created on: Oct 13, 2017
 *      Author: iotto
 */

#include "xwalk/runtime/renderer/android/xwalk_render_frame_ext.h"

#include <map>
#include <string>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/autofill_agent.h"
#include "components/autofill/content/renderer/password_autofill_agent.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_hit_test_result.h"
#include "third_party/blink/public/web/web_image_cache.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_meaningful_layout.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/public/web/web_view.h"
#include "url/url_canon.h"
#include "url/url_constants.h"
#include "url/url_util.h"
#include "xwalk/runtime/common/android/xwalk_hit_test_data.h"
#include "xwalk/runtime/common/android/xwalk_render_view_messages.h"

#include "meta_logging.h"

namespace xwalk {

namespace {

const char kAddressPrefix[] = "geo:0,0?q=";
const char kEmailPrefix[] = "mailto:";
const char kPhoneNumberPrefix[] = "tel:";


GURL GetAbsoluteUrl(const blink::WebElement& element,
                    const base::string16& url_fragment) {
  return GURL(element.GetDocument().CompleteURL(blink::WebString::FromUTF16(url_fragment)));
}

base::string16 GetHref(const blink::WebElement& element) {
  // Get the actual 'href' attribute, which might relative if valid or can
  // possibly contain garbage otherwise, so not using absoluteLinkURL here.
  return element.GetAttribute("href").Utf16();
}

GURL GetAbsoluteSrcUrl(const blink::WebElement& element) {
  if (element.IsNull())
    return GURL();
  return GetAbsoluteUrl(element, element.GetAttribute("src").Utf16());
}

blink::WebElement GetImgChild(const blink::WebNode& node) {
  // This implementation is incomplete (for example if is an area tag) but
  // matches the original WebViewClassic implementation.

  blink::WebElementCollection collection = node.GetElementsByHTMLTagName("img");
  DCHECK(!collection.IsNull());
  return collection.FirstItem();
}

GURL GetChildImageUrlFromElement(const blink::WebElement& element) {
  const blink::WebElement child_img = GetImgChild(element);
  if (child_img.IsNull())
    return GURL();
  return GetAbsoluteSrcUrl(child_img);
}

bool RemovePrefixAndAssignIfMatches(const base::StringPiece& prefix,
                                    const GURL& url,
                                    std::string* dest) {
  const base::StringPiece spec(url.possibly_invalid_spec());

  if (spec.starts_with(prefix)) {
    url::RawCanonOutputW<1024> output;
    url::DecodeURLEscapeSequences(spec.data() + prefix.length(),
                                  spec.length() - prefix.length(),
                                  url::DecodeURLMode::kUTF8OrIsomorphic,&output);
    *dest =
        base::UTF16ToUTF8(base::StringPiece16(output.data(), output.length()));
    return true;
  }
  return false;
}

void DistinguishAndAssignSrcLinkType(const GURL& url, XWalkHitTestData* data) {
  if (RemovePrefixAndAssignIfMatches(kAddressPrefix, url,
                                     &data->extra_data_for_type)) {
    data->type = XWalkHitTestData::GEO_TYPE;
  } else if (RemovePrefixAndAssignIfMatches(kPhoneNumberPrefix, url,
                                            &data->extra_data_for_type)) {
    data->type = XWalkHitTestData::PHONE_TYPE;
  } else if (RemovePrefixAndAssignIfMatches(kEmailPrefix, url,
                                            &data->extra_data_for_type)) {
    data->type = XWalkHitTestData::EMAIL_TYPE;
  } else {
    data->type = XWalkHitTestData::SRC_LINK_TYPE;
    data->extra_data_for_type = url.possibly_invalid_spec();
    if (!data->extra_data_for_type.empty()) {
      data->href = base::UTF8ToUTF16(data->extra_data_for_type);
    }
  }
}

void PopulateHitTestData(const GURL& absolute_link_url,
                         const GURL& absolute_image_url, bool is_editable,
                         XWalkHitTestData* data) {
  // Note: Using GURL::is_empty instead of GURL:is_valid due to the
  // WebViewClassic allowing any kind of protocol which GURL::is_valid
  // disallows. Similar reasons for using GURL::possibly_invalid_spec instead of
  // GURL::spec.
//  LOG(INFO) << __func__ << " absolute_link_url=" << absolute_link_url
//      << " absolute_image_url=" << absolute_image_url;
  if (!absolute_image_url.is_empty())
    data->img_src = absolute_image_url;

  const bool is_javascript_scheme = absolute_link_url.SchemeIs(
      url::kJavaScriptScheme);
  const bool has_link_url = !absolute_link_url.is_empty();
  const bool has_image_url = !absolute_image_url.is_empty();

  if (has_link_url && !has_image_url && !is_javascript_scheme) {
    DistinguishAndAssignSrcLinkType(absolute_link_url, data);
  } else if (has_link_url && has_image_url && !is_javascript_scheme) {
    data->type = XWalkHitTestData::SRC_IMAGE_LINK_TYPE;
    data->extra_data_for_type = data->img_src.possibly_invalid_spec();
    if (absolute_link_url.is_valid()) {
      data->href = base::UTF8ToUTF16(absolute_link_url.possibly_invalid_spec());
    }
  } else if (!has_link_url && has_image_url) {
    data->type = XWalkHitTestData::IMAGE_TYPE;
    data->extra_data_for_type = data->img_src.possibly_invalid_spec();
  } else if (is_editable) {
    data->type = XWalkHitTestData::EDIT_TEXT_TYPE;
    DCHECK_EQ(data->extra_data_for_type.length(), 0u);
  }
}

}  // namespace

// Registry for RenderFrame => AwRenderFrameExt lookups
typedef std::map<content::RenderFrame*, XWalkRenderFrameExt*> FrameExtMap;
base::LazyInstance<FrameExtMap>::Leaky render_frame_ext_map =
    LAZY_INSTANCE_INITIALIZER;

XWalkRenderFrameExt::XWalkRenderFrameExt(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  // TODO(sgurun) do not create a password autofill agent (change
  // autofill agent to store a weakptr).
  autofill::PasswordAutofillAgent* password_autofill_agent =
      new autofill::PasswordAutofillAgent(render_frame, &registry_);
  new autofill::AutofillAgent(render_frame, password_autofill_agent, nullptr,
                              &registry_);

  // Add myself to the RenderFrame => AwRenderFrameExt register.
  render_frame_ext_map.Get().emplace(render_frame, this);
}

XWalkRenderFrameExt::~XWalkRenderFrameExt() {
  // Remove myself from the RenderFrame => AwRenderFrameExt register. Ideally,
  // we'd just use render_frame() and erase by key. However, by this time the
  // render_frame has already been cleared so we have to iterate over all
  // render_frames in the map and wipe the one(s) that point to this
  // XWalkRenderFrameExt

  auto& map = render_frame_ext_map.Get();
  auto it = map.begin();
  while (it != map.end()) {
    if (it->second == this) {
      it = map.erase(it);
    } else {
      ++it;
    }
  }
}

bool XWalkRenderFrameExt::OnAssociatedInterfaceRequestForFrame(const std::string& interface_name,
                                                               mojo::ScopedInterfaceEndpointHandle* handle) {
  return registry_.TryBindInterface(interface_name, handle);
}

//// static
//void XWalkRenderFrameExt::RenderViewCreated(content::RenderView* render_view) {
//  new XWalkRenderFrameExt(render_view);  // |render_view| takes ownership.
//}

bool XWalkRenderFrameExt::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(XWalkRenderFrameExt, message)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_DocumentHasImages,
                        OnDocumentHasImagesRequest)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_DoHitTest, OnDoHitTest)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_SetTextZoomLevel, OnSetTextZoomLevel)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_ResetScrollAndScaleState,
                        OnResetScrollAndScaleState)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_SetInitialPageScale, OnSetInitialPageScale)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_SetBackgroundColor, OnSetBackgroundColor)
    IPC_MESSAGE_HANDLER(XWalkViewMsg_SetTextZoomFactor, OnSetTextZoomFactor)
    //TODO missing ; see AwRederFrameExt
//    IPC_MESSAGE_HANDLER(AwViewMsg_SmoothScroll, OnSmoothScroll)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void XWalkRenderFrameExt::OnDocumentHasImagesRequest(uint32_t id) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  // AwViewMsg_DocumentHasImages should only be sent to the main frame.
  DCHECK(frame);
  DCHECK(!frame->Parent());

  const blink::WebElement child_img = GetImgChild(frame->GetDocument());
  bool has_images = !child_img.IsNull();

  Send(new XWalkViewHostMsg_DocumentHasImagesResponse(routing_id(), id,
                                                      has_images));
}

void XWalkRenderFrameExt::DidCommitProvisionalLoad(bool is_same_document_navigation,
                                                   ui::PageTransition transition) {

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  content::DocumentState* document_state =
      content::DocumentState::FromDocumentLoader(frame->GetDocumentLoader());
  if (document_state->can_load_local_resources()) {
    frame->GetDocument().GrantLoadLocalResources();
  }

  // Clear the cache when we cross site boundaries in the main frame.
  //
  // We're trying to approximate what happens with a multi-process Chromium,
  // where navigation across origins would cause a new render process to spin
  // up, and thus start with a clear cache. Wiring up a signal from browser to
  // renderer code to say "this navigation would have switched processes" would
  // be disruptive, so this clearing of the cache is the compromise.
  if (!frame->Parent()) {
    url::Origin new_origin = url::Origin::Create(frame->GetDocument().Url());
    if (!new_origin.IsSameOriginWith(last_origin_)) {
      last_origin_ = new_origin;
      blink::WebImageCache::Clear();
    }
  }
}

void XWalkRenderFrameExt::FocusedElementChanged(const blink::WebElement& element) {
  if (element.IsNull() || !render_frame() || !render_frame()->GetRenderView())
    return;

  XWalkHitTestData data;

  data.href = GetHref(element);
  data.anchor_text = element.TextContent().Utf16();

  GURL absolute_link_url;
  if (element.IsLink())
    absolute_link_url = GetAbsoluteUrl(element, data.href);

  GURL absolute_image_url = GetChildImageUrlFromElement(element);

  TENTA_LOG(INFO) << "iotto " << __func__ << " link=" << absolute_link_url << " IsContentEditable="
                  << element.IsContentEditable();

  PopulateHitTestData(absolute_link_url, absolute_image_url,
                      element.IsEditable(), &data);
  Send(new XWalkViewHostMsg_UpdateHitTestData(routing_id(), data));
}

void XWalkRenderFrameExt::OnDestruct() {
  delete this;
}

void XWalkRenderFrameExt::OnDoHitTest(const gfx::PointF& touch_center,
                                      const gfx::SizeF& touch_area) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  const blink::WebHitTestResult result = webview->HitTestResultForTap(
      blink::WebPoint(touch_center.x(), touch_center.y()),
      blink::WebSize(touch_area.width(), touch_area.height()));
  XWalkHitTestData data;

  GURL absolute_image_url = result.AbsoluteImageURL();
  if (!result.UrlElement().IsNull()) {
    data.anchor_text = result.UrlElement().TextContent().Utf16();
    data.href = GetHref(result.UrlElement());
    // If we hit an image that failed to load, Blink won't give us its URL.
    // Fall back to walking the DOM in this case.
    if (absolute_image_url.is_empty()) {
      absolute_image_url = GetChildImageUrlFromElement(result.UrlElement());
    }
  }

  TENTA_LOG(INFO) << __func__ << " link=" << result.AbsoluteLinkURL() << " IsContentEditable="
      << result.IsContentEditable();
  PopulateHitTestData(result.AbsoluteLinkURL(), absolute_image_url,
                      result.IsContentEditable(), &data);
  Send(new XWalkViewHostMsg_UpdateHitTestData(routing_id(), data));
}

void XWalkRenderFrameExt::OnSetTextZoomLevel(double zoom_level) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  // Hide selection and autofill popups.
  webview->CancelPagePopup();
  webview->SetZoomLevel(zoom_level);
}

void XWalkRenderFrameExt::OnResetScrollAndScaleState() {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->ResetScrollAndScaleState();
}

void XWalkRenderFrameExt::OnSetInitialPageScale(double page_scale_factor) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->SetInitialPageScaleOverride(page_scale_factor);
}

void XWalkRenderFrameExt::OnSetBackgroundColor(SkColor c) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->SetBaseBackgroundColor(c);
}

void XWalkRenderFrameExt::OnSetTextZoomFactor(float zoom_factor) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  // Hide selection and autofill popups.
  webview->CancelPagePopup();
  webview->SetTextZoomFactor(zoom_factor);
}

blink::WebView* XWalkRenderFrameExt::GetWebView() {
  if (!render_frame() || !render_frame()->GetRenderView() ||
      !render_frame()->GetRenderView()->GetWebView())
    return nullptr;

  return render_frame()->GetRenderView()->GetWebView();
}

blink::WebFrameWidget* XWalkRenderFrameExt::GetWebFrameWidget() {
  return render_frame()
             ? render_frame()->GetWebFrame()->LocalRoot()->FrameWidget()
             : nullptr;
}
} /* namespace xwalk */
