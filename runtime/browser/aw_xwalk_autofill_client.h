/*
 * aw_xwalk_autofill_client.h
 *
 *  Created on: Dec 9, 2019
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_AW_XWALK_AUTOFILL_CLIENT_H_
#define XWALK_RUNTIME_BROWSER_AW_XWALK_AUTOFILL_CLIENT_H_

#include "android_webview/browser/aw_autofill_client.h"

namespace xwalk {

class AwXwalkAutofillClient : public android_webview::AwAutofillClient {
 public:
  ~AwXwalkAutofillClient() override;

 protected:
  AwXwalkAutofillClient(content::WebContents* web_contents);
  friend class content::WebContentsUserData<AwXwalkAutofillClient>;
};

} /* namespace xwalk */

#endif /* XWALK_RUNTIME_BROWSER_AW_XWALK_AUTOFILL_CLIENT_H_ */
