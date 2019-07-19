cmake_minimum_required(VERSION 2.8) 

PROJECT(WifiManager)

add_definitions("-std=c++11") 

INCLUDE_DIRECTORIES(../include)

AUX_SOURCE_DIRECTORY(. WifiManager)

ADD_LIBRARY (WifiManager SHARED ${WifiManager})

TARGET_LINK_LIBRARIES(WifiManager pthread)
