/*
 * Copyright 2019-2020 LightComm.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
 
#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <iostream>
#include <string>
#include <memory>
#include <thread>

#include <SDKInterfaces/WifiStatusObserverInterface.h>

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

