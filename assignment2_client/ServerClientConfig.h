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


#define RANDOM_SEED 1	// 生成随机数作为处理数据

// Data: 数据分为MAX_THREADS=64 块，每块SUBDATANUM个数据。整体数据分为两半，client处理前一半数据 （64*CLIENT_SUBDATANUM），server处理后一半数据（64*SERVER_SUBDATANUM）
#define MAX_THREADS     3          // 线程数：64
#define SUBDATANUM      20     // 子块数据量：2000000
//#define MAX_THREADS     64          // 线程数：64
//#define SUBDATANUM      2000000     // 子块数据量：2000000
#define DATANUM         (SUBDATANUM*MAX_THREADS)  // 总数据量：线程数x子块数据量
#define SERVER_SUBDATANUM  10     // gaurantee SERVER_SUBDATANUM+CLIENT_SUBDATANUM == SUBDATANUM
#define CLIENT_SUBDATANUM  10    // 单PC数据：1000000
#define SERVER_DATANUM	(SERVER_SUBDATANUM * MAX_THREADS)
#define CLIENT_DATANUM	(CLIENT_SUBDATANUM * MAX_THREADS)


// Network
#define SERVER_ADDRESS "127.0.0.1"//Server端IP地址
#define SERVER_PORT 12341

#define S_ONCE              100                         // 一次发送100个double
#define S_TIMES             (S_CLT_DATANUM / S_ONCE)      // 总共发送100个的次数
#define S_LEFT              (S_CLT_DATANUM % S_ONCE)      // 发送剩余不足的数据

// Variables
float* rawFloatData = new float[DATANUM];
int ThreadID[MAX_THREADS];
float floatResults[MAX_THREADS];//每个线程的中间结果
// max/sum result
float finalSumResult;                    // 求和最终结果
float finalMaxResult;                    // 求最大值最终结果

// sort result
float SortTotalResult[DATANUM];             // 排序最终结果
#ifdef SERVER
float ServerSortResult[MAX_THREADS][SERVER_SUBDATANUM];     // Server 各块排序结果
float ServerSortMergedResult[SERVER_DATANUM];
float ClientSortResult[CLIENT_DATANUM];

#endif

#endif
