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
#define MAX_THREADS     32          // 线程数：64
#define SUBDATANUM      1000000     // 子块数据量：2000000  tips: 生成随机数，有很多0，ok的
#define DATANUM         (SUBDATANUM*MAX_THREADS)  // 总数据量：线程数x子块数据量
#define SERVER_SUBDATANUM  500000     // gaurantee SERVER_SUBDATANUM+CLIENT_SUBDATANUM == SUBDATANUM
#define CLIENT_SUBDATANUM  500000    // 单PC数据：1000000
#define SERVER_DATANUM	(SERVER_SUBDATANUM * MAX_THREADS)
#define CLIENT_DATANUM	(CLIENT_SUBDATANUM * MAX_THREADS)

// Network
#define SERVER_ADDRESS "127.0.0.1"//Server端IP地址
#define SERVER_PORT 12341

#define S_ONCE              100                         // 一次发送100个float
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
#ifdef SERVER
float SortTotalResult[DATANUM];             // server+client 最终排序结果
float ServerSortResult[MAX_THREADS][SERVER_SUBDATANUM];     // Server 各块排序结果
float ServerSortMergedResult[SERVER_DATANUM];
#endif
#ifdef CLIENT
float ClientSortResult[MAX_THREADS][SERVER_SUBDATANUM];
#endif
float ClientSortMergedResult[CLIENT_DATANUM];	// client 排序结果（server，client 端均需要）


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






// ========================= SORT==============================

void quickSort(float* data, int lowIndex, int highIndex) {
	int i = lowIndex, j = highIndex;
	float tmp_data = data[i];

	while (i < j) {
		while (i < j and data[j] > tmp_data) { j--; };
		if (i < j) {
			data[i++] = data[j];
		}
		while (i < j and data[i] <= tmp_data) { i++; };
		if (i < j) {
			data[j--] = data[i];
		}
	}
	data[i] = tmp_data;

	if (lowIndex < i - 1) { quickSort(data, lowIndex, i - 1); }
	if (highIndex > i + 1) { quickSort(data, i + 1, highIndex); }

}


/* Merge 2 array
*/
void merge_2_sorted_array(float* sorted_first_array, int first_array_len, float* sorted_second_array, int second_array_len, SORTED_ARRAY_MERGE* sort_result_array) {

	int i = 0, j = 0, total_array_index = 0;
	while (i < first_array_len && j < second_array_len) {
		if (sorted_first_array[i] <= sorted_second_array[j]) {
			sort_result_array->merged_array[total_array_index] = sorted_first_array[i];
			i++;
		}
		else {
			sort_result_array->merged_array[total_array_index] = sorted_second_array[j];
			j++;
		}
		total_array_index++;
	}
	while (i < first_array_len) {
		sort_result_array->merged_array[total_array_index] = sorted_first_array[i];
		i++;
		total_array_index++;
	}
	while (j < second_array_len) {
		sort_result_array->merged_array[total_array_index] = sorted_second_array[j];
		j++;
		total_array_index++;
	}
	sort_result_array->merged_len = first_array_len + second_array_len;

}

// check sort result: ascend
int check_sorted_result(const float data[], const int len) {
	for (int i = 0; i < len - 1; i++) {
		if (data[i + 1] < data[i])
			return 0;
	}
	return 1;
}



#endif
