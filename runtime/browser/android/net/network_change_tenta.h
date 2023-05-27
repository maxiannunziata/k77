/*
 * network_change_tenta.h
 *
 *  Created on: Feb 2, 2017
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_NET_NETWORK_CHANGE_TENTA_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_NET_NETWORK_CHANGE_TENTA_H_

#include "base/lazy_instance.h"
#include "net/base/network_change_notifier.h"

namespace xwalk {
namespace tenta {

class NetworkChangeTenta : public net::NetworkChangeNotifier::IPAddressObserver,
    public net::NetworkChangeNotifier::ConnectionTypeObserver,
    public net::NetworkChangeNotifier::DNSObserver {
 public:
  static NetworkChangeTenta * GetInstance();

 private:
  friend struct base::LazyInstanceTraitsBase<NetworkChangeTenta>;

  NetworkChangeTenta();
  ~NetworkChangeTenta() override;

  // from net::NetworkChangeNotifier::IPAddressObserver:
  void OnIPAddressChanged() override;

  // from net::NetworkChangeNotifier::ConnectionTypeObserver:
  void OnConnectionTypeChanged(net::NetworkChangeNotifier::ConnectionType type)
      override;

  // from net::NetworkChangeNotifier::DNSObserver:
  void OnDNSChanged() override;
  void OnInitialDNSConfigRead() override;

};

} /* namespace tenta */
} /* namespace xwalk */

#endif /* XWALK_RUNTIME_BROWSER_ANDROID_NET_NETWORK_CHANGE_TENTA_H_ */
