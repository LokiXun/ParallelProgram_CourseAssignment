/*
* Ԥ����ͷ�ļ�
*/

#pragma once

#ifndef PCH_H
#define PCH_H

//�������
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>

//SSE���
#include <emmintrin.h>  //open look,look 128bit
#include <immintrin.h>  //open look,look 256bit
#include <zmmintrin.h>  //open look,look 512bit

//�������
#include<iostream>
#include<string>
#include<Windows.h>
#include <map>


#define RANDOM_SEED 1	// �����������Ϊ��������

// Data: ���ݷ�ΪMAX_THREADS=64 �飬ÿ��SUBDATANUM�����ݡ��������ݷ�Ϊ���룬client����ǰһ������ ��64*CLIENT_SUBDATANUM����server�����һ�����ݣ�64*SERVER_SUBDATANUM��
#define MAX_THREADS     3          // �߳�����64
#define SUBDATANUM      20     // �ӿ���������2000000
//#define MAX_THREADS     64          // �߳�����64
//#define SUBDATANUM      2000000     // �ӿ���������2000000
#define DATANUM         (SUBDATANUM*MAX_THREADS)  // �����������߳���x�ӿ�������
#define SERVER_SUBDATANUM  10     // gaurantee SERVER_SUBDATANUM+CLIENT_SUBDATANUM == SUBDATANUM
#define CLIENT_SUBDATANUM  10    // ��PC���ݣ�1000000
#define SERVER_DATANUM	(SERVER_SUBDATANUM * MAX_THREADS)
#define CLIENT_DATANUM	(CLIENT_SUBDATANUM * MAX_THREADS)


// Network
#define SERVER_ADDRESS "127.0.0.1"//Server��IP��ַ
#define SERVER_PORT 12341

#define S_ONCE              100                         // һ�η���100��double
#define S_TIMES             (S_CLT_DATANUM / S_ONCE)      // �ܹ�����100���Ĵ���
#define S_LEFT              (S_CLT_DATANUM % S_ONCE)      // ����ʣ�಻�������

// Variables
float* rawFloatData = new float[DATANUM];
int ThreadID[MAX_THREADS];
float floatResults[MAX_THREADS];//ÿ���̵߳��м���
// max/sum result
float finalSumResult;                    // ������ս��
float finalMaxResult;                    // �����ֵ���ս��

// sort result
float SortTotalResult[DATANUM];             // �������ս��
#ifdef SERVER
float ServerSortResult[MAX_THREADS][SERVER_SUBDATANUM];     // Server ����������
float ServerSortMergedResult[SERVER_DATANUM];
float ClientSortResult[CLIENT_DATANUM];

#endif

#endif
