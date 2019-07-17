## WifiManager
嵌入式平台下基于 wpa_supplicant 工具实现wifi管理

## 解决的问题
1. 监测wifi的状态，很多公司是通过在应用层ping路由器的方式来实现，这种方式不是非常实时，处理很麻烦，也很容易出错
* 本工具通过wpa_supplicant提供的wpa_ctrl接口，通过本地socket与wpa_supplicant通讯，实时获取wifi状态信息

## 测试平台
* MT8516

