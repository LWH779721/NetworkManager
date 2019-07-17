/******************************************************************************
LEGAL DISCLAIMER

(Header of MediaTek Software/Firmware Release or Documentation)

Mediatek Inc. © 2017-2018. All rights reserved.

File name: wpa_ctrl.c
Author: yali.wu
Description: This file contains function for communicating with wpa_supplicant.
Others:
******************************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/un.h>
#include "unistd.h"
#include <malloc.h>

#include "SmartLink/wpa_ctrl.h"


/**
 * struct WpaCtrl - Internal structure for control interface library
 *
 * This structure is used by the wpa_supplicant/hostapd control interface
 * library to store internal data. Programs using the library should not touch
 * this data directly. They can only use the pointer to the data structure as
 * an identifier for the control interface connection and use this as one of
 * the arguments for most of the control interface library functions.
 */
struct WpaCtrl
{
    int s;
    struct sockaddr_un mLocal;
    struct sockaddr_un mDest;
};

static int wpaShowErrno(void)
{
    //WIFI_LOG(WIFI_ERROR, ("errno: %d, %s\n", errno, strerror(errno)));
    return (0);
}

struct WpaCtrl *wpaCtrlOpen(const char *kCtrlPath, const char *kBindPath)
{
    struct WpaCtrl *pCtrl = NULL;
    int bindRet = 0;
    //int ret = -1;
    int connectRet = 0;
    //char buf[64] = {'\0'};

    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Enter %s: ctrl_path = %s.",
    //    __FUNCTION__, kCtrlPath));

    pCtrl = (struct WpaCtrl *)malloc(sizeof(*pCtrl));
    if (NULL == pCtrl)
    {
        return NULL;
    }
	memset(pCtrl, 0, sizeof(*pCtrl));

    pCtrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (pCtrl->s < 0)
    {
        //WIFI_LOG(WIFI_DEBUG, ("[ERR] %s: socket() fail, ctrl->s: %d.",
        //    __FUNCTION__, pCtrl->s));
		free(pCtrl);
        return NULL;
    }

    pCtrl->mLocal.sun_family = AF_UNIX;

    snprintf(pCtrl->mLocal.sun_path, sizeof(pCtrl->mLocal.sun_path),
        "%s", kBindPath);

    //WIFI_LOG(WIFI_DEBUG, ("%s: unlink(), ctrl->s: %d, ctrl->mLocal.sun_path: %s",
    //           __FUNCTION__, pCtrl->s, pCtrl->mLocal.sun_path));
    unlink(kBindPath);

    bindRet = bind(pCtrl->s, (struct sockaddr *)(&pCtrl->mLocal),
        sizeof(pCtrl->mLocal));
    if (bindRet < 0)
    {
        wpaShowErrno();
        close(pCtrl->s);
		free(pCtrl);
        //WIFI_LOG(WIFI_DEBUG, ("[ERR] %s: bind() fail.", __FUNCTION__));
        return NULL;
    }

    //WIFI_LOG(WIFI_DEBUG, ("%s: bind(), bindRet = %d.", __FUNCTION__, bindRet));
    pCtrl->mDest.sun_family = AF_UNIX;
    snprintf(pCtrl->mDest.sun_path, sizeof(pCtrl->mDest.sun_path),
        "%s", kCtrlPath);

    //WIFI_LOG(WIFI_DEBUG, ("%s: connect(), ctrl->s: %d, ctrl->dest.sun_path: %s",
    //           __FUNCTION__, pCtrl->s, pCtrl->mDest.sun_path));

    connectRet = (int)(connect(pCtrl->s, (struct sockaddr *)(&(pCtrl->mDest)),
        sizeof(pCtrl->mDest)));
    if (connectRet < 0)
    {
        wpaShowErrno();
        close(pCtrl->s);
        unlink(pCtrl->mLocal.sun_path);
		free(pCtrl);
        //WIFI_LOG(WIFI_DEBUG, ("[ERR] %s: connect() fail, connectRet = %d.",
        //    __FUNCTION__, connectRet));
        return NULL;
    }
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Leave %s(), conn = %d.",
    //    __FUNCTION__, connectRet));

    return pCtrl;
}

void wpaCtrlClose(struct WpaCtrl *ctrl)
{
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Enter %s(), ctrl->s: %d",
    //    __FUNCTION__, ctrl->s));

    unlink(ctrl->mLocal.sun_path);
    close(ctrl->s);

    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Leave %s(), ctrl->s: %d",
    //    __FUNCTION__, ctrl->s));
	free(ctrl);
}

int wpaCtrlRequest(struct WpaCtrl *ctrl, const char *kCmd, size_t cmdLen,
                     char *reply, size_t *replyLen,
                     void (*msgCb)(void *msg, size_t len))
{
    struct timeval tv;
    int retValue = -1;
    fd_set readFds;
    const char *kCmdStr = NULL;
    size_t cmdStrLen = 0;
    int sendSocket = ctrl->s;

    if(!ctrl || !kCmd || !reply || !replyLen)
    {
        //WIFI_LOG(WIFI_ERROR, ("%s: Invalid Param!!", __FUNCTION__));
    }

    kCmdStr = kCmd;
    cmdStrLen = cmdLen;
    //WIFI_LOG(WIFI_DEBUG, ("%s: socket: %d, [cmd=%s], len=%d.", __FUNCTION__, 
    //    ctrl->s, kCmdStr, cmdStrLen));

    if (send(sendSocket, kCmdStr, cmdStrLen, 0) < 0)
    {
        //WIFI_LOG(WIFI_DEBUG, ("send() fail, errorno=%d, @%s()",
        //    errno, __FUNCTION__));
        return -1;
    }

    for (;;)
    {
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        //WIFI_LOG(WIFI_INFO, ("set select timeout %d, @%s()", tv.tv_sec,
        //    __FUNCTION__));

        FD_ZERO(&readFds);
        FD_SET(sendSocket, &readFds);
        retValue = select(sendSocket + 1, &readFds, NULL, NULL, &tv);
        if (FD_ISSET(sendSocket, &readFds))
        {
            retValue = recv(sendSocket, reply, *replyLen, 0);
            if (retValue > 0 && '<' == reply[0])
            {
                //WIFI_LOG(WIFI_DEBUG, ("unsolicited message\n"));
                /* This is an unsolicited message from
                 * wpa_supplicant, not the reply to the
                 * request. Use msg_cb to report this to the
                 * caller. */
                if (msgCb)
                {
                    /* Make sure the message is null terminated. */
                    if ((size_t)retValue == *replyLen)
                    {
                        retValue = (*replyLen) - 1;
                    }
                    reply[retValue] = '\0';
                    msgCb(reply, retValue);
                }
                continue;
            }
            else if (retValue < 0)
            {
                //WIFI_LOG(WIFI_DEBUG, ("recv() fail, @%s()", __FUNCTION__));
                return retValue;
            }
            *replyLen = retValue;
            break;
        }
        else
        {
            //WIFI_LOG(WIFI_DEBUG, ("select() timeout, @%s()", __FUNCTION__));
            return -2;
        }
    }
    return 0;
}

static int wpaCtrlAttachHelper(struct WpaCtrl *ctrl, int attach)
{
    char buf[10] = {'\0'};
    int retValue = -1;
    size_t bufLen = sizeof(buf);

    //WIFI_LOG_ENTRY(WIFI_API, (""));

    retValue = wpaCtrlRequest(ctrl, attach ? "ATTACH" : "DETACH", 6,
                           buf, &bufLen, NULL);

    if (3 == bufLen && 0 == memcmp(buf, "OK\n", 3))
    {
        return 0;
    }
    else if (retValue < 0)
    {
        return retValue;
    }

    return -1;
}

int wpaCtrlAttach(struct WpaCtrl *ctrl)
{
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Enter %s.", __FUNCTION__));

    return wpaCtrlAttachHelper(ctrl, 1);
}

int wpaCtrlDetach(struct WpaCtrl *ctrl)
{
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Enter %s.", __FUNCTION__));

    return wpaCtrlAttachHelper(ctrl, 0);
}

int wpaCtrlRecv(struct WpaCtrl *ctrl, void *reply, size_t *replyLen)
{
    int resVal = -1;

    resVal = recv(ctrl->s, reply, *replyLen, 0);
    if (resVal >= 0)
    {
        *replyLen = resVal;
    }
    else
    {
        return resVal;
    }

    return 0;
}

int wpaCtrlSendNfyRet(struct WpaCtrl *ctrl, void *cmd, size_t cmdLen)
{
    int sendRet = 0;
#if 0
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] %s: Before send() ctrl->s: %d, "
        "mNotifyId: %d, i4Ret: %d, cmd_len: %d.\n",
        __FUNCTION__, ctrl->s, (int)((Net80211Notify *)cmd)->mNotifyId,
        (int)((Net80211Notify *)cmd)->mRet, cmdLen));
#endif
    sendRet = (int)(send(ctrl->s, cmd, cmdLen, 0));
    if (sendRet < 0)
    {
        wpaShowErrno();
        //WIFI_LOG(WIFI_DEBUG, ("[ERR] %s, send fail, sendRet: %d.",
        //    __FUNCTION__, sendRet));
        return -1;
    }
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] %s, After send(), sendRet: %d.",
    //    __FUNCTION__, sendRet));

    return 0;
}

int wpaCtrlPending(struct WpaCtrl *ctrl)
{
    struct timeval tv;
    fd_set rFds;

    if (NULL == ctrl)
    {
        //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Enter %s, ctrl = null!",
        //    __FUNCTION__));
        return -1;
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rFds);
    FD_SET(ctrl->s, &rFds);
    select(ctrl->s + 1, &rFds, NULL, NULL, &tv);
    return FD_ISSET(ctrl->s, &rFds);
}

int wpaCtrlGetFd(struct WpaCtrl *ctrl)
{
    //WIFI_LOG(WIFI_DEBUG, ("[WPA_CTRL] Enter %s(CTRL_IFACE_SOCKET).",
    //    __FUNCTION__));

    return ctrl->s;
}


