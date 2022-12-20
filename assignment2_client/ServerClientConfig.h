/*
* 预编译头文件
*/

#pragma once

#ifndef PCH_H
#define PCH_H

//网络相关
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>

//SSE相关
#include <emmintrin.h>  //open look,look 128bit
#include <immintrin.h>  //open look,look 256bit
#include <zmmintrin.h>  //open look,look 512bit

//功能相关
#include<iostream>
#include<string>
#include<Windows.h>
#include <map>



#endif
