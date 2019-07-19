/*
 * Copyright 2017-2018 LightComm.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef LIGHT_COMM_SPEAKER_AVSCOMMON_SDKINTERFACES_WIFI_STATUS_OBSERVERE_INTERFACE_H_
#define LIGHT_COMM_SPEAKER_AVSCOMMON_SDKINTERFACES_WIFI_STATUS_OBSERVERE_INTERFACE_H_


namespace LightCommSpeaker {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A WifiStatusObserverInterface is an interface class that clients can extend to be notified of the status of wifi
 */
class WifiStatusObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~WifiStatusObserverInterface() = default;

    enum Status {
        IDLE,
        AP,
        DISCONNECTED,
		CONNECTING,
        CONNECTED,
        UNKNOWN,
    };


    /**
     */
    virtual void onWifiStatusChanged(const Status newstatus) = 0;
    
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace LightCommSpeaker

#endif  // LIGHT_COMM_SPEAKER_AVSCOMMON_SDKINTERFACES_WIFI_STATUS_OBSERVERE_INTERFACE_H_

