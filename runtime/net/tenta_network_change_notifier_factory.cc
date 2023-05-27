/*
 * network_change_notifier_factory_tenta.cc
 *
 *  Created on: Dec 5, 2016
 *      Author: iotto
 */

#include "xwalk/runtime/net/tenta_network_change_notifier_factory.h"
#include "net/android/network_change_notifier_android.h"
#include "net/android/network_change_notifier_delegate_android.h"

namespace xwalk {
namespace tenta {

NetworkChangeNotifierFactoryTenta::NetworkChangeNotifierFactoryTenta() {}

NetworkChangeNotifierFactoryTenta::~NetworkChangeNotifierFactoryTenta() {}

//net::NetworkChangeNotifier* NetworkChangeNotifierFactoryTenta::CreateInstance() {
//  return new net::NetworkChangeNotifierAndroid(&delegate_, nullptr);
//}

}  // namespace tenta
}  // namespace xwalk
