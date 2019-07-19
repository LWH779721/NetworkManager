#include <iostream>
#include <string>
#include <cstring>
#include <algorithm> 
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

#include "WifiManager.h"
#include "wpa_ctrl.h"

using namespace std;

namespace LightCommSpeaker {
namespace smartLink {

bool WifiManager::genApConfig(const string &ssid, const string &passwd){
	FILE *src = NULL, *dst = NULL;
	char line[1024] = {0};
	
	if (NULL == (src = fopen("/etc/hostapd_lightcomm.conf", "r"))
		|| NULL == (dst = fopen("/etc/tmp.conf", "w"))){
		cout << "open config failed" << endl;
		return false;
	}

	while (fgets(line, 1024, src)){
		if (feof(src)){
			break;
		}

		if ((0 == strncmp(line, "ssid", 4))
			&& (ssid.length() != 0)){
			strcpy(line + 5, ssid.c_str());
			strcat(line, "\n");
		}

		if ((0 == strncmp(line, "wpa_passphrase", 14))
			&& (passwd.length() != 0)){
			strcpy(line + 15, passwd.c_str());
			strcat(line, "\n");
		}

		fputs(line, dst);
	}

	fclose(src);
	fclose(dst);
	rename("/etc/tmp.conf", "/etc/hostapd_lightcomm.conf");
	
	return true;
}

bool WifiManager::setupAp(){	
	if (m_apSetuped){
		cout << "Soft ap already setup" << endl;
		return true;
	}

	m_apSetuped = true;
    system("killall wpa_supplicant");
    system("killall udhcpc");
	system("echo AP > /dev/wmtWifi");

    system("ifconfig wlan0 down");
	system("ifconfig ap0 up");
	system("ifconfig ap0 192.168.1.1 netmask 255.255.255.0");
	system("mkdir -p /var/lib/misc");
	system("touch /var/lib/misc/udhcpd.leases");
	system("udhcpd /etc/udhcpd.conf&");
	system("hostapd -dd /etc/hostapd_lightcomm.conf&");

    m_observer->onWifiStatusChanged(LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::AP);
    m_wifiStatus = LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::AP;
    
	return true;
}

bool WifiManager::shutdownAp(){
	if (!m_apSetuped){
		cout << "Soft ap already stop" << endl;
		return true;
	}

	m_apSetuped = false;
	system("killall udhcpd");
	system("ip addr delete 192.168.1.1 dev ap0 &");
	system("killall hostapd");
	system("ifconfig ap0 down");
    system("ifconfig wlan0 up");

    m_observer->onWifiStatusChanged(LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::IDLE);
    m_wifiStatus = LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::IDLE;
    system("wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf -B");
    system("udhcpc -i wlan0 -s /usr/share/udhcpc/mtkdhcp.script -p /tmp/udhcpc-wlan0.pid -n -t 20 -T 2 -x hostname:bws02l-consys-slc-128-32b &");

    m_monitorThread = std::thread(&WifiManager::monitor, this); 
	m_monitorThread.detach();
    
	return true;
}

WifiManager::WifiManager(const string &apSsid, const string &apPasswd):m_apSetuped(false),
    m_wifiStatus(LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::IDLE){
    
	m_apSsid = apSsid;
	m_apPasswd = apPasswd;

	genApConfig(m_apSsid, m_apPasswd);

    system("wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf -B");
    system("udhcpc -i wlan0 -s /usr/share/udhcpc/mtkdhcp.script -p /tmp/udhcpc-wlan0.pid -n -t 20 -T 2 -x hostname:bws02l-consys-slc-128-32b &");

    m_monitorThread = std::thread(&WifiManager::monitor, this); 
	m_monitorThread.detach();
}

void WifiManager::monitor(){
    struct WpaCtrl *monitorConn = NULL;
	char buf[1000];
	int ret;
    size_t nRead;
    
	monitorConn = wpaCtrlOpen("/tmp/wpa_supplicant/wlan0", "/tmp/wpa_supplicant/wpa_moni_455");
	if (NULL == monitorConn){
		cout << "wpaCtrlOpen failed" << endl;
		return;
	}
	
	if (wpaCtrlAttach(monitorConn) != 0){
		cout << "wpaCtrlAttach failed" << endl;
		goto exit;
		return;
	}
	
	while (1){ 
        nRead = sizeof(buf) - 1;
        ret = wpaCtrlRecv(monitorConn, buf, &nRead);
        if (ret < 0){
            continue;
        }

        buf[nRead] = '\0';
        if (0 == ret && 0 == nRead){
            cout << "wpa_supplicant killed" << endl;
            break;
        }
            
		//cout << "recv: " << buf << endl;
		
		if (strstr(buf, "TERMINATING")){
			cout << "wpa_supplicant killed" << endl;
            break;
		} else if (strstr(buf, "CTRL-EVENT-CONNECTED")){
            cout << "wifi CONNECTED" << endl;
            if (m_wifiStatus != LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::CONNECTED){
                m_wifiStatus = LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::CONNECTED;
                m_observer->onWifiStatusChanged(
                    LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::CONNECTED);
            }
        } else if (strstr(buf, "CTRL-EVENT-DISCONNECTED")) {
            cout << "wifi disCONNECTED" << endl;
            if (m_wifiStatus != LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::DISCONNECTED){
                m_wifiStatus = LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::DISCONNECTED;
                m_observer->onWifiStatusChanged(
                    LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::DISCONNECTED);
            }
        }
	}

exit:
    wpaCtrlClose(monitorConn);
}

bool WifiManager::reConfigAp(const string &apSsid, const string &apPasswd){
	if ((m_apSsid == apSsid) 
		&& (m_apPasswd == apPasswd)){
		cout << "config is same" << endl;
		return false;
	}

	m_apSsid = apSsid;
	m_apPasswd = apPasswd;
	genApConfig(m_apSsid, m_apPasswd);
    
	if (m_apSetuped){
		shutdownAp();
		setupAp();
	}

	return true;
}

bool WifiManager::genWpaConfig(const string &ssid, const string &passwd){
    FILE *dst = NULL;
	string tmp;

    if (ssid.length() == 0){
        cout << "ssid err" << endl;
		return false;    
    }

	if (NULL == (dst = fopen("/etc/tmp.conf", "w"))){
		cout << "create config failed" << endl;
		return false;
	}

    fputs("ctrl_interface=/tmp/wpa_supplicant\n", dst);
    fputs("update_config=1\n\n", dst);
    
    fputs("network={\n", dst);

    tmp = "     ssid=\"" + ssid + "\"\n";
    fputs(tmp.c_str(), dst);

    if (passwd.length() != 0){      
	    tmp = "     psk=\"" + passwd + "\"\n";
        fputs(tmp.c_str(), dst);
    } else {
        fputs("     psk=\"\"\n", dst);   
        fputs("     key_mgmt=NONE\n", dst); 
    }
    
    fputs("     priority=100\n", dst); 
    fputs("}\n", dst);

	fclose(dst); 

    return true;
}

bool WifiManager::wifiConnect(const string &ssid, const string &passwd){
    genWpaConfig(ssid, passwd);
    rename("/etc/tmp.conf", "/etc/wpa_supplicant.conf");
    
    system("wpa_cli -iwlan0 -p/tmp/wpa_supplicant reconfigure");
    system("killall udhcpc");
    system("udhcpc -i wlan0 -s /usr/share/udhcpc/mtkdhcp.script -p /tmp/udhcpc-wlan0.pid -n -t 20 -T 2 -x hostname:bws02l-consys-slc-128-32b &");

    if (m_wifiStatus != LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::CONNECTING){
        m_wifiStatus = LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::CONNECTING;
        m_observer->onWifiStatusChanged(
            LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface::CONNECTING);
    }
        
    return true;
}

bool WifiManager::setObserver(std::shared_ptr<LightCommSpeaker::avsCommon::sdkInterfaces::WifiStatusObserverInterface> observer){
    m_observer = observer;
    
    return true;
}

}
}

