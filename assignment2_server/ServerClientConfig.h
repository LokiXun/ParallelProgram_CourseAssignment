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
#define DATANUM         (SUBDATANUM*MAX_THREADS)  // 总数据量：线程数x子块数据量
#define SERVER_SUBDATANUM  10     // gaurantee SERVER_SUBDATANUM+CLIENT_SUBDATANUM == SUBDATANUM
#define CLIENT_SUBDATANUM  10    // 单PC数据：1000000
#define SERVER_DATANUM	(SERVER_SUBDATANUM * MAX_THREADS)
#define CLIENT_DATANUM	(CLIENT_SUBDATANUM * MAX_THREADS)


// Network
#define SERVER_ADDRESS "127.0.0.1"//Server端IP地址
#define SERVER_PORT 12341

#define S_ONCE              100                         // 一次发送100个double
#define S_TIMES             (CLIENT_DATANUM / S_ONCE)      // 总共发送100个的次数
#define S_LEFT              (CLIENT_DATANUM % S_ONCE)      // 发送剩余不足的数据
#define BUF_LEN				1024            // 文字缓存长度：1kB
char buf[BUF_LEN];		// 网络收发缓存




// Variables
float* rawFloatData = new float[DATANUM];
int ThreadID[MAX_THREADS];
float floatResults[MAX_THREADS]; // each thread's maxResult in server


// max/sum result
float finalSumResult;                    // 求和最终结果
float finalMaxResult;                    // 求最大值最终结果


// sort result
float SortTotalResult[DATANUM];             // server+client 最终排序结果
#ifdef SERVER
float ServerSortResult[MAX_THREADS][SERVER_SUBDATANUM];     // Server 各块排序结果
float ServerSortMergedResult[SERVER_DATANUM];
float ClientSortResult[CLIENT_DATANUM];
#endif


// Other
enum class Method {
	MT_SUM = 1,
	MT_MAX = 2,
	MT_SORT = 3,
	WAIT = 0,
	END = 9
};


typedef struct _thread_data {
	int thread_id;
	const float* data;
	int start_index;
	int end_index;
} THREAD_DATA;

typedef struct _thread_data_sort {
	int thread_id;
	int start_index;
	int block_data_len;
} THREAD_DATA_SORT;

typedef struct merge_sorted_array {
	float* merged_array;
	int merged_len;
} SORTED_ARRAY_MERGE;


#endif
