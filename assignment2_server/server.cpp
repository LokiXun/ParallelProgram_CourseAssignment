/*
* Server端主程序
*/


#pragma region macro_definition

#define _WINSOCK_DEPRECATED_NO_WARNINGS//因为用到了过时方法inet_addr()，没这个过不了编译
#define MAX_THREADS 2//最大线程数
#define SUBDATANUM 10//各线程处理数据长度
#define DATANUM (SUBDATANUM * MAX_THREADS)   /*这个数值是总数据量*/
#define SERVER_ADDRESS "127.0.0.1"//Server端IP地址
#define SERVER_PORT 10086
#define RANDOM_SEED 1



#pragma endregion



#pragma region header

#include"ServerClientConfig.h"//pch为预编译头文件

#pragma endregion header



#pragma region global_variable
//自定义类
enum class Method {
	MT_SUM = 1,
	MT_MAX = 2,
	MT_SORT = 3,
	WAIT = 0,
	END = 9
};


//SSE加速的方法共享同一网络连接
//我想通过连接当参数传入来实现封装，但是老师把函数参数定死了，就只能用全局变量了

WSAData wsaData;//socket库信息
WORD dllVersion;//dllVersion信息
SOCKADDR_IN addr;//网络地址信息
int addrlen = sizeof(addr);//addr数据长度
SOCKET sListen;
SOCKET servConnection;//servConnection为服务端连接

int ThreadID[MAX_THREADS];
float floatResults[MAX_THREADS];//每个线程的中间结果
float** sortResultArraySequece = new float* [MAX_THREADS]; // server 排序结果
HANDLE hSemaphores[MAX_THREADS];//信号量，保证不重入。等同于mutex
typedef struct _thread_data {
	int thread_id;
	const float* data;
	int start_index;
	int end_index;
} THREAD_DATA;

typedef struct _thread_data_sort {
	int thread_id;
	int start_index;
	int end_index;
} THREAD_DATA_SORT;

typedef struct merge_sorted_array {
	float* merged_array;
	int merged_len;
} SORTED_ARRAY_MERGE;

#pragma endregion



#pragma region method_declaraion
//单机版：服务器端不提供（本架构里服务器只进行被动响应）

//双机加速版本
//采取多线程和SSE技术加速
int initSrvSocket();//初始化SSE
int closeSocket();//关闭SSE
float sumSpeedUp(const float data[], const int len); //data是原始数据，len为长度。结果通过函数返回
float maxSpeedUp(const float data[], const int len);//data是原始数据，len为长度。结果通过函数返回
float sortSpeedUp(const float data[], const int len, float result[]);//data是原始数据，len为长度。排序结果在result中。

#pragma endregion



//主函数
int main() {
	using namespace std;

	//初始化
	srand(RANDOM_SEED);
	float* rawFloatData = new float[DATANUM];
	float* sortSourceArray = new float[DATANUM];
	float* sortResultsArray = new float[DATANUM];

	for (size_t i = 0; i < DATANUM; i++) {//数据初始化
		rawFloatData[i] = fabs(double(rand()));  // float(i + 1);
		sortSourceArray[i] = rawFloatData[i];
	}
	float* result = new float[DATANUM] {0};//存放排序过的float数组
	float ret = 0.0f;

	for (size_t i = 0; i < min(DATANUM, 30); i++) {
		std::cout << rawFloatData[i] << " ";
	}
	std::cout << std::endl;
	
	//循环控制
	int lpFlag;


	//初始化网络连接
	initSrvSocket();


	//测试循环

	lpFlag = 0;
	enum Method mtd = Method::WAIT;
	while (lpFlag == 0) {
		//TODO:总之这部分仅仅是实现了功能，对于连接中断后的处理，连接的多线程化以及线程池管理等都没有做。
		recv(servConnection, (char*)&mtd, sizeof(int), NULL);//接收指令
		cout << "Client message received.";
		switch (mtd) {
		case Method::MT_SUM:
			cout << "Testing sumSpeedup  method..." << endl;
			ret = sumSpeedUp(rawFloatData, DATANUM);
			cout << "The result is " << ret << endl;
			break;
		case Method::MT_MAX:
			cout << "Testing maxSpeedup method..." << endl;
			ret = maxSpeedUp(rawFloatData, DATANUM);
			cout << "The result is " << ret << endl;
			break;
		case Method::MT_SORT:
			cout << "Testing sortSpeedup method..." << endl;
			sortSpeedUp(rawFloatData, DATANUM, sortResultsArray);
			break;
		case Method::END:
			lpFlag = -1;
			break;
		default:
			break;
		}
		mtd = Method::WAIT;
	}


	//关闭网络连接
	closeSocket();

}


#pragma region method_defination
/*
* 初始化网络连接
*/
int initSrvSocket() {

	//!使用WSAStartup函数绑定对应的socket库
	std::cout << "Windows Socket startup..." << std::endl;
	dllVersion = MAKEWORD(2, 1);//调用2.1版本的dll
	if (WSAStartup(dllVersion, &wsaData) != 0) {//初始化并检验是否初始化成功
		MessageBoxA(NULL, "Windows Socket startup error.", "Error", MB_OK | MB_ICONERROR);//MessgaeBox弹出错误信息
		return -1;//失败则返回-1
	}

	//!使用 SOCKADDR_IN 变量配置 Server 端的网络地址
	std::cout << "Configurating server IP&Port..." << std::endl;
	addr.sin_family = AF_INET;//配置IP地址族：AF_INET：UDP、TCP协议
	addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);//Server端IP地址
	addr.sin_port = htons(SERVER_PORT);//Server端端口号

	//!创建监听SOCKET，并等待请求
	std::cout << "Configuration server listener..." << std::endl;
	sListen = socket(AF_INET, SOCK_STREAM, NULL);//socket函数返回对应配置的socket对象实例
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));//将sListen绑定值addr所配置的网络地址上
	listen(sListen, SOMAXCONN);//设置sListen为监听模式，等待连接请求

	//请求到来后，建立连接
	std::cout << "Waiting for client request..." << std::endl;
	servConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);//servConnection接受sListen
	std::cout << "Connection is established." << std::endl;

	return 0;
}


/*
* 回收SSE资源
*/
int closeSocket() {

	//回收资源
	closesocket(servConnection);//回收newConnection的资源
	WSACleanup();//回收网络服务架构配置的资源
	return 0;
}


/*
* Sum : Speed Up Using SSE + OpenMP
* Func 对数据部分范围 [startIndex，endIndex] 闭区间, 求和。按 8 个数分块，使用SSE 256bit，最后一块不足 8 个对的部分就直接加
* @data 待求和的float一维数组
* @stInd
* @len float数组长度
* @return 数组data全元素的和
*/
float SumArray_speedUp(const float data[], const int startIndex, const int endIndex) {

	if (endIndex >= DATANUM) {
		throw std::exception("Sum func, endIndex >= DATANUM");
	}
	if (endIndex < startIndex) {
		throw std::exception("Sum func, endIndex<startIndex");
	}

	int part_array_len = endIndex - startIndex + 1;
	for (int i = startIndex; i <= endIndex; i++) {
		std::cout << data[i] << std::endl;
	}


	//SSE加速 
	int SSE_parallel_num = 8; //float为32bit，则256位可以一次处理256/32=8个float
	int sse_iter = int(floor(part_array_len / SSE_parallel_num)); // 防止不能整除
	const float* stPtr = data + startIndex;

	float** retSum = new float* [sse_iter];
	for (int i = 0; i < sse_iter; ++i) {
		retSum[i] = new float[SSE_parallel_num] { 0 };
	}
	float* retSum2 = new float[sse_iter] {0};
	float ret = 0.0f;

#pragma omp parallel for//omp多线程加速
	for (int i = 0; i < sse_iter; ++i) {
		__m256* ptr = (__m256*)stPtr + i * SSE_parallel_num;//每个线程的起始点为 data+i*单个线程循环次数
		for (int j = 0; j < sse_iter; ++j, ++ptr) {//单个线程循环次数为sse_iter/最大线程数，起始只要保证整数就行
			//_mm256_store_ps(retSum[i], _mm256_add_ps(*(__m256*)retSum[i], _mm256_log_ps(_mm256_sqrt_ps((*ptr)))));//SSE指令套娃
			_mm256_store_ps(retSum[i], _mm256_add_ps(*(__m256*)retSum[i], *ptr));//SSE指令套娃
		}
	}
	//整合结果
#pragma omp parallel for
	for (int i = 0; i < sse_iter; ++i) {
		for (int j = 0; j < SSE_parallel_num; ++j) {
			retSum2[i] += retSum[i][j];
		}
		delete[] retSum[i];//顺道回收内存
	}

	for (int i = 0; i < sse_iter; i++) {
		ret += retSum2[i];
		std::cout << "retSum2[i]=" << retSum2[i] << std::endl;
		std::cout << "ret=" << ret << std::endl;
	}

	// 处理最后一个未整除的块
	for (int i = startIndex + sse_iter * SSE_parallel_num; i <= endIndex; ++i) {
		//ret += log(sqrt(data[i]));
		std::cout << "data[i]=" << data[i] << "ret=" << ret << std::endl;
		ret += data[i];

	}

	//回收动态数组内存
	delete[] retSum;
	delete[] retSum2;

	return ret;
}

/**
* 数组求和：server 分发一半数据给 client，之后在 server 整合数据
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素的和
*/
float sumSpeedUp(const float data[], const int len) {

	float ret;
	float* retSum = new float[2] { 0 };
	int ind = int(len / 2) - 1;//ind指client段所需计算到的下标

	// 0. 任务分配
	// 目前只有2台机器，发送前一半给client计算，后一半自己算
	send(servConnection, (char*)&ind, sizeof(int), NULL);

	//1. server 算计算另一半 TODO: 调用client里面的同一个函数
	retSum[0] = SumArray_speedUp(data, ind + 1, len - 1);

	//2. 接收Client端的结果
	recv(servConnection, (char*)&retSum[1], sizeof(float), NULL);

	//3. 整合结果
	ret = retSum[0] + retSum[1];
	std::cout << "Sum SpeedUp Calculate Finished! reesult = " << ret << std::endl;

	////把最终结果返回Client端（Depend）
	//send(servConnection, (char*)&ret, sizeof(ret), NULL);

	return ret;
}

/*
* 求最大值加速版（单机版） TODO
* @data 待求和的float一维数组
* @endIndex 数组待处理范围的末尾元素下标
* @return 数组data全元素中最大值
*/
float singleSpdMax(const float data[], const int stInd, const int len) {
	//局部变量
	float ret;
	float retMax[8];//用于SSE加速的中间存储
	int sse_iter = len / 8;//SSE迭代总长度
	const float* stPtr = data + stInd;//偏移后的数组首地址
	//最大值赋初值
	_mm256_store_ps(retMax, _mm256_log_ps(_mm256_sqrt_ps(*(__m256*)stPtr)));

	//TODO:双机加速
#pragma omp parallel for//omp多线程加速
	for (int i = 0; i < MAX_THREADS; ++i) {
		__m256* ptr = (__m256*)stPtr + i * sse_iter / MAX_THREADS;
		//SSE加速
		for (int j = 0; j < sse_iter / MAX_THREADS; ++j, ++ptr) {
			_mm256_store_ps(retMax, _mm256_max_ps(*(__m256*)retMax, _mm256_log_ps(_mm256_sqrt_ps((*ptr)))));
		}
	}

	//整合结果
	ret = retMax[0];
	for (int i = 0; i < 8; ++i) {
		if (ret < retMax[i]) {
			ret = retMax[i];
		}
	}

	return ret;
}

/*
* 求最大值 多线程版本
* @data 待求和的float一维数组
* @endIndex 数组待处理范围的末尾元素下标
* @return 数组data全元素中最大值
*/
DWORD WINAPI maxArray_multithread(LPVOID lpParameter)
{
	THREAD_DATA* threadData = (THREAD_DATA*)lpParameter;

	float max_value = threadData->data[threadData->start_index];
	for (int i = threadData->start_index; i <= (threadData->end_index); i++)
	{
		max_value = max_value >= threadData->data[i] ? max_value : threadData->data[i];//
	}
	floatResults[threadData->thread_id] = max_value;
	ReleaseSemaphore(hSemaphores[threadData->thread_id], 1, NULL);//释放信号量，信号量加1 

	return 0;
}

/**
* 加速版求最大值 多线程
* @data 待求最大元素的float一维数组
* @len float数组长度
* @return 数组data各元素中的最大值
*/
float maxSpeedUp(const float data[], const int len) {
	float ret;
	float* retMax = new float[2] { 0 };
	int ind = int(len / 2) - 1;//ind指client段所需计算到的下标

	// 0. 任务分配
	send(servConnection, (char*)&ind, sizeof(int), NULL);


	//1. server 计算后面一半
	LARGE_INTEGER start_time, end_time, time_freq;
	QueryPerformanceFrequency(&time_freq);
	QueryPerformanceCounter(&start_time);//start  
	/*					multi-thread									*/
	int waited_array_start_index = int(ind) + 1;
	int CURRENT_THREAD_NUM = min(MAX_THREADS, len - waited_array_start_index); // 当前线程数，如果数据量 < MAX_THREADS 不需要开这么多
	int each_thread_process_num = ceil((len - waited_array_start_index) / CURRENT_THREAD_NUM);

	THREAD_DATA* current_thread_data = new THREAD_DATA[MAX_THREADS];
	HANDLE hThreads[MAX_THREADS];
	// 1.1 create thread task
	for (int i = 0; i < MAX_THREADS; i++)
	{
		hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价？
		ThreadID[i] = i;
		floatResults[i] = 0;

		// 线程输入数据
		current_thread_data[i].thread_id = i;
		current_thread_data[i].data = data;
		current_thread_data[i].start_index = min(waited_array_start_index + i * each_thread_process_num, len - 1);
		current_thread_data[i].end_index = min(current_thread_data[i].start_index + each_thread_process_num - 1, len - 1);
		floatResults[i] = data[waited_array_start_index];  // set default

		hThreads[i] = CreateThread(
			NULL,// default security attributes
			0,// use default stack size
			maxArray_multithread,// thread function
			&current_thread_data[i],// argument to thread function
			CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
			NULL);

		if (hThreads[i] == NULL)
		{
			printf("WARN: hThreads=%d CreateThread error: %d\n", i, GetLastError());
			throw std::exception("WARN: getMax CreateThread error!");
		}
	}
	// 1.2.execute + wait
	for (int i = 0; i < MAX_THREADS; i++)
	{
		ResumeThread(hThreads[i]);
	}
	std::cout << "Satrt WaitForMultipleObjects for 2mins!" << std::endl;
	DWORD ThreadFinished = WaitForMultipleObjects(MAX_THREADS, hSemaphores, TRUE, 2 * 60 * 1000);
	if (!((WAIT_OBJECT_0 <= ThreadFinished) && (ThreadFinished <= (WAIT_OBJECT_0 + MAX_THREADS - 1)))) {
		// exception
		printf("WARN: thread end exception! ThreadFinished=%d", ThreadFinished);
		throw std::exception("WARN: getMax thread end exception!");
	}
	delete[] current_thread_data;


	// 1.3 get result
	retMax[0] = data[waited_array_start_index];
	for (int i = 0; i < MAX_THREADS; i++)
		retMax[0] = retMax[0] > floatResults[i] ? retMax[0] : floatResults[i];

	QueryPerformanceCounter(&end_time);//start 
	std::cout << "Server Get Maxiumu finished! max_value=" << retMax[0] <<
		", costs = " << ((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart << "s" << std::endl;

	//2. 接收Client端的结果
	recv(servConnection, (char*)&retMax[1], sizeof(float), NULL);

	//3. 整合结果
	ret = retMax[0] > retMax[1] ? retMax[0] : retMax[1];
	QueryPerformanceCounter(&end_time);//start 
	std::cout << "Max SpeedUp: result =" << ret << ", costs = " <<
		((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart << "s" << std::endl;

	return ret;
}


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

/*
* 排序算法加速版（单机版）
* @data 待求和的float一维数组
* @parameter stInd - 待排序的起始下标
* @parameter data - 待排序的数组
* @parameter len - 待排序的长度
* @return 数组data排序后的结果
*/
float singleSpdSort(const float data[], const int stInd, const int len, float result[]) {
	float* res = new float[8] { 0 };//SSE加速的中间
	int sse_iter = len / 8;//SSE迭代总长度
	const float* stPtr = data + stInd;//偏移后的数组首地址

#pragma omp parallel for
	for (int i = 0; i < MAX_THREADS; ++i) {
		__m256* ptr = (__m256*)stPtr + i * sse_iter / MAX_THREADS;//这是SSE加速的分段首地址
		//SSE加速
		for (int j = 0; j < sse_iter / MAX_THREADS; ++j, ++ptr) {
			//TODO:排序算法

		}
	}

	return 0.0f;
}

/*
* 排序 multithread
*/
DWORD WINAPI sortArray_multithread(LPVOID lpParameter)
{
	THREAD_DATA* threadData = (THREAD_DATA*)lpParameter;
	int current_array_len = threadData->end_index - threadData->start_index +1;
	quickSort(sortResultArraySequece[threadData->thread_id], 0, current_array_len - 1); //float** sortResultArraySequece = new float * [Num]

	ReleaseSemaphore(hSemaphores[threadData->thread_id], 1, NULL);//释放信号量，信号量加1 
	return 0;
}


/* Merge 2 array
*/
void merge_2_sorted_array(float* sorted_first_array, int first_array_len, float* sorted_second_array, SORTED_ARRAY_MERGE* sort_result_array) {
	int second_array_len = _msize(sorted_second_array) / sizeof(sorted_second_array[0]);

	int i = 0, j = 0, total_array_index = 0;
	while( i < first_array_len && j < second_array_len) {
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

/* check sort result: ascend
*/
int check_sorted_result(const float data[], const int len) {
	for (int i = 0; i < len -1; i++) {
		if (data[i + 1] < data[i]) 
			return 0;
	}
	return 1;
}

/**
* 加速版排序
* @data 待排序的float一维数组
* @len float数组长度
* @return 若正常求解，则返回0.0
*/
float sortSpeedUp(const float data[], const int len, float result[]) {
	// TIPS: TCP在单机网上一次最大传输65536字节，即16384个float, 8192个double
	// 在局域网内根据网卡，一次最大传输1500字节，即375个float, 187个double

	int firstHafEndIndex = int(floor(len / 2) - 1); //ind指client段所需计算到的下标
	int secondHalfLen = len - firstHafEndIndex - 1;

	float* sortResultArray = new float[len] {0};
	float* clientResult = new float[firstHafEndIndex + 1] {0};
	float* serverResult = new float[secondHalfLen] {0};

	// 0. 任务分配
	send(servConnection, (char*)&firstHafEndIndex, sizeof(int), NULL);

	//1. server 对后一半排序
	LARGE_INTEGER start_time, end_time, time_freq;
	QueryPerformanceFrequency(&time_freq);
	QueryPerformanceCounter(&start_time);//start  
	int waited_array_start_index = int(firstHafEndIndex) + 1;
	int each_thread_process_num = int(max(ceil(secondHalfLen / MAX_THREADS), 2)); // 每个线程最少处理2个数据
	int CURRENT_THREAD_NUM = int(ceil((len - waited_array_start_index) / each_thread_process_num)); // 当前需要的线程数，保证每个线程有数据可以处理

	THREAD_DATA_SORT* current_thread_data = new THREAD_DATA_SORT[CURRENT_THREAD_NUM];
	HANDLE hThreads[MAX_THREADS];
	// 1.1 create thread task
	for (int i = 0; i < MAX_THREADS; i++)
	{
		hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (i >= CURRENT_THREAD_NUM) { continue; }

		// 线程输入数据
		current_thread_data[i].thread_id = i;
		current_thread_data[i].start_index = min(waited_array_start_index + i * each_thread_process_num, len - 1);
		current_thread_data[i].end_index = min(current_thread_data[i].start_index + each_thread_process_num - 1, len - 1);
		int current_block_len = current_thread_data[i].end_index - current_thread_data[i].start_index + 1;
		// cut each block's data
		sortResultArraySequece[i] = new float[current_block_len];
		printf("Block No.%d\n", i);
		for (int tmp_index=0; (tmp_index + current_thread_data[i].start_index) <= current_thread_data[i].end_index; tmp_index++) {
			sortResultArraySequece[i][tmp_index] = data[tmp_index + current_thread_data[i].start_index];
			std::cout << sortResultArraySequece[i][tmp_index] << " ";
		}
		std::cout << std::endl;
		

		hThreads[i] = CreateThread(
			NULL,// default security attributes
			0,// use default stack size
			sortArray_multithread,// thread function
			&current_thread_data[i],// argument to thread function
			CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
			NULL);

		if (hThreads[i] == NULL)
		{
			printf("WARN: SORT hThreads=%d CreateThread error: %d\n", i, GetLastError());
			throw std::exception("WARN: SORT CreateThread error!");
		}
	}
	// 1.2.execute + wait
	for (int i = 0; i < CURRENT_THREAD_NUM; i++)
	{
		ResumeThread(hThreads[i]);
	}
	std::cout << "Start WaitForMultipleObjects for 2mins!" << std::endl;
	DWORD ThreadFinished = WaitForMultipleObjects(CURRENT_THREAD_NUM, hSemaphores, TRUE, 2 * 60 * 1000);
	if (!((WAIT_OBJECT_0 <= ThreadFinished) && (ThreadFinished <= (WAIT_OBJECT_0 + MAX_THREADS - 1)))) {
		// exception
		printf("WARN: thread end exception! ThreadFinished=%d", ThreadFinished);
		throw std::exception("WARN: getMax thread end exception!");
	}
	std::cout << "Sort multihthread Finish!" << std::endl;
	delete[] current_thread_data;
	
	for (int i = 0; i < CURRENT_THREAD_NUM; i++) {
		int data_len = _msize(sortResultArraySequece[i]) / sizeof(sortResultArraySequece[i][0]);
		printf("sortResultArraySequece No.%d/%d ,data_len=%d\n", i, CURRENT_THREAD_NUM, data_len);
		for (int j = 0; j < data_len; j++) {
			std::cout << sortResultArraySequece[i][j] << " ";
		}
		std::cout << std::endl;
	}
	

	// 1.3 merge server's result(Current in dumb ways: merge each 2 sequence)
	SORTED_ARRAY_MERGE* tmp_array_data = new SORTED_ARRAY_MERGE;
	SORTED_ARRAY_MERGE* merge_result_data = new SORTED_ARRAY_MERGE;
	tmp_array_data->merged_array = new float[secondHalfLen]{0};
	tmp_array_data->merged_len = 0;
	merge_result_data->merged_array = new float[secondHalfLen]{0};
	merge_result_data->merged_len = 0;
	if (CURRENT_THREAD_NUM == 1) {
		// only use one thread could deal with all the data
		for (int i = 0; i < secondHalfLen; i++) {
			merge_result_data->merged_array[i] = sortResultArraySequece[0][i];
		}
		merge_result_data->merged_len = secondHalfLen;
	}
	else {
		// 多个线程数据
		int current_merge_array_no = 1;
		while (current_merge_array_no < CURRENT_THREAD_NUM) {
			// initialize tmp_array >> merge (tmp_array, sortResultArraySequece[current_merge_array_no])
			if (current_merge_array_no == 1) {
				int first_array_len = _msize(sortResultArraySequece[0]) / sizeof(sortResultArraySequece[0][0]);
				merge_2_sorted_array(sortResultArraySequece[0], first_array_len, sortResultArraySequece[current_merge_array_no], merge_result_data);
			}
			else {
				// merge from last time merge result
				for (int i = 0; i < merge_result_data->merged_len; i++) {
					tmp_array_data->merged_array[i] = merge_result_data->merged_array[i];
				}
				tmp_array_data->merged_len = merge_result_data->merged_len;

				merge_2_sorted_array(tmp_array_data->merged_array, tmp_array_data->merged_len, sortResultArraySequece[current_merge_array_no], merge_result_data);
			}
			current_merge_array_no++;
		}
	}

	int sort_success_flag = check_sorted_result(merge_result_data->merged_array, merge_result_data->merged_len);
	QueryPerformanceCounter(&end_time);//start 
	std::cout << "Server SORT finished! sort_success=" << sort_success_flag <<
		", costs = " << ((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart << "s" << std::endl;
	for (int i = 0; i < merge_result_data->merged_len; i++) {
		std::cout << merge_result_data->merged_array[i] << " ";
	}
	std::cout << std::endl;

	//2. 接收Client端的结果
	recv(servConnection, (char*)&clientResult, (firstHafEndIndex + 1) * sizeof(float), NULL);

	//3. 整合结果
	int total_array_index = 0;
	int i = 0, j = 0;

	return 0.0;
}


#pragma endregion