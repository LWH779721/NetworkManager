#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <iostream>
#include <string>
#include <memory>
#include <thread>

#include <WifiStatusObserverInterface.h>

using namespace std;

namespace LightCommSpeaker {
namespace smartLink {

/*
 *  for wifi manage
 */
class WifiManager {
public:
	WifiManager(const string &apSsid, const string &apPasswd);
    
	bool reConfigAp(const string &apSsid, const string &apPasswd);
    
	bool setupAp();
    
	bool shutdownAp();

    bool wifiConnect(const string &ssid, const string &passwd);

    bool genWpaConfig(const string &ssid, const string &passwd);

    bool setObserver(std::shared_ptr<LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface> observer);
    
private:
    string m_apSsid;
    
    string m_apPasswd;

    bool m_apSetuped;
    
    LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::Status m_wifiStatus;
        
    std::thread m_monitorThread;
    
    std::shared_ptr<LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface> m_observer;
    
    bool genApConfig(const string &ssid, const string &passwd); 

    void monitor();
};

}
}

#endif //__WIFI_MANAGER_H__

