/*
* Client端主程序
*/

#define CLIENT
#define _WINSOCK_DEPRECATED_NO_WARNINGS//因为用到了过时方法inet_addr()，没这个过不了编译
#include "ServerClientConfig.h"//pch为预编译头文件


#pragma region global_variable
WSAData wsaData;//socket库信息
WORD dllVersion;//dllVersion信息
SOCKADDR_IN addr;//网络地址信息
int addrlen = sizeof(addr);//addr数据长度
SOCKET cltConnection;//cltConnection为服务端连接
HANDLE hSemaphores[MAX_THREADS];//信号量，保证不重入。等同于mutex
int ret, recv_len, send_len, sin_size; // 一些网络收发相关变量
#pragma endregion




#pragma region method_defination
/*
* 初始化网络连接
*/
int initCltSocket() {

	//!使用WSAStartup函数绑定对应的socket库
	std::cout << "Windows Socket startup..." << std::endl;
	dllVersion = MAKEWORD(2, 1);//调用2.1版本的dll
	if (WSAStartup(dllVersion, &wsaData) != 0) {//初始化并检验是否初始化成功
		MessageBoxA(NULL, "Windows Socket startup error.", "Error", MB_OK | MB_ICONERROR);//MessgaeBox弹出错误信息
		return -1;//失败则返回-1
	}

	//!使用SOCKADDR_IN变量配置所要连接的Server端的网络地址
	std::cout << "Configurating IP&Port of the server to connect..." << std::endl;
	addr.sin_family = AF_INET;//配置IP地址族：AF_INET：UDP、TCP协议
	addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);//Server端IP地址
	addr.sin_port = htons(SERVER_PORT);//Server端端口号

	//向Server端发送请求，并创建连接
	cltConnection = socket(AF_INET, SOCK_STREAM, NULL);//
	std::cout << "Sending request to the server..." << std::endl;
	if (connect(cltConnection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {//创建连接并检验是否成功
		MessageBoxA(NULL, "Bad Connection", "Error", MB_OK | MB_ICONERROR);//在MessageBox窗口上打印错误信息
		return -1; //终止
	}
	std::cout << "Connection is established." << std::endl;

	return 0;
}


/*
* 回收Socket资源
*/
int closeSocket() {

	//回收资源
	closesocket(cltConnection);//回收newConnection的资源
	WSACleanup();//回收网络服务架构配置的资源
	std::cout << "Connection is cleaned up." << std::endl;
	return 0;
}

/**
* 非加速版求和
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素的和
*/
float SimpleSum(const float data[], const int len) {
	double result = 0.0f;
	for (int i = 0; i < len; i++) {
		//result += log10(sqrt(data[i] / 4.0));
		result += data[i];
	}

	return result;
}


float myMax(const float data[], const int len) {
	//float ret = log(sqrt(data[0]));
	float ret = data[0];
	for (int i = 0; i < len; ++i) {
		//float value = log(sqrt(data[i]));
		if (ret < data[i]) {
			ret = data[i];
		}
	}

	return ret;
}



//=================== SORT =====================
/*
* sort函数采用快排
*/
float sort(const float data[], const int len, float result[]) {
	//深复制data
	for (int i = 0; i < len; ++i) {
		//result[i] = log(sqrt(data[i]));
		result[i] = data[i];
	}

	//快速排序
	quickSort(result, 0, len - 1);

	return 0.0f;
}


// =============================================== SUM
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

#pragma omp parallel for//omp多线程加速: 计算 sqrt+log + 分块
	for (int i = 0; i < sse_iter; ++i) {
		__m256* ptr = (__m256*)(stPtr + i * SSE_parallel_num);//每个线程的起始点为 data+i*单个线程循环次数
		_mm256_store_ps(retSum[i], _mm256_add_ps(*(__m256*)retSum[i], *ptr));//SSE指令套娃
		//_mm256_store_ps(retSum[i], _mm256_add_ps(*(__m256*)retSum[i], _mm256_log_ps(_mm256_sqrt_ps((*ptr)))));
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
	}

	// 处理最后一个未整除的块
	for (int i = startIndex + sse_iter * SSE_parallel_num; i <= endIndex; ++i) {
		//ret += log(sqrt(data[i]));
		ret += data[i];
		//std::cout << "data[i]=" << data[i] << "ret=" << ret << std::endl;
	}

	//回收动态数组内存
	delete[] retSum;
	delete[] retSum2;

	return ret;
}



/**
* 加速版求和
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素的和
*/
float sumSpeedUp(const float data[], const int len) {
	float ret;
	int firstHalfLen; // client 处理前面一半数据，len 为数据长度
	enum Method mtd = Method::MT_SUM;
	//向Server端发送分布运算请求
	send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
	//获得任务分配: 前半段数组的结尾下标
	recv(cltConnection, (char*)&firstHalfLen, sizeof(int), NULL);
	//求解分任务
	ret = SumArray_speedUp(data, 0, firstHalfLen - 1);
	std::cout << "Current Client Sum result = " << ret << std::endl;
	//把结果发送给Server端整合
	send(cltConnection, (char*)&ret, sizeof(ret), NULL);
	std::cout << "Send2Server success!" << std::endl;
	//从Server端得到最终结果
	recv(cltConnection, (char*)&ret, sizeof(ret), NULL);

	return ret;
}

// =============================================== MAX
/*
* 求最大值加速版（单机版）
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素中最大值
*/
extern "C"
__declspec(dllexport) float singleSpdMax(const float data[], const int stInd, const int len) {
	//局部变量
	float ret;

	//SSE加速
	int sse_parallel_num = 8;//256位同时计算8个float
	int sse_iter = int(floor(len / sse_parallel_num));//SSE迭代总长度
	int sse_leave_task = len - sse_parallel_num * sse_iter;//sse遗留任务
	float* retMax = new float[sse_parallel_num] { 0 };//用于SSE加速的中间存储
	const float* stPtr = data + stInd;//偏移后的数组首地址

	if (len < sse_parallel_num) {//对于sse都不能用的情况直接遍历并返回
		ret = log(sqrt(data[0]));
		for (int i = 0; i < len; ++i) {
			float value = log(sqrt(data[i]));
			if (ret < value) {
				ret = value;
			}
		}
		return ret;
	}

	//多线程参数
	int thread_num;//实际线程数
	int min_thread_task = 32;//每个线程的最少任务数
	int thread_task;//每个线程的任务
	int thread_leave_task;//多线程遗留任务

	if (sse_iter < min_thread_task) {
		thread_num = 1;
		thread_task = sse_iter;
	}
	else {
		thread_num = int(floor(sse_iter / min_thread_task));
		thread_num = min(MAX_THREADS, thread_num);//不得大于最大线程数
		thread_task = int(floor(sse_iter / thread_num));
	}
	thread_leave_task = sse_iter - thread_num * thread_task;


	//最大值赋初值
	_mm256_store_ps(retMax, _mm256_log_ps(_mm256_sqrt_ps(*(__m256*)stPtr)));

#pragma omp parallel for//omp多线程加速
	for (int i = 0; i < thread_num; ++i) {
		__m256* ptr = (__m256*)stPtr + i * thread_task;
		//SSE加速
		for (int j = 0; j < thread_task; ++j, ++ptr) {
			_mm256_store_ps(retMax, _mm256_max_ps(*(__m256*)retMax, _mm256_log_ps(_mm256_sqrt_ps((*ptr)))));
		}
	}

	//多线程剩余任务存入retSum第一行
	__m256* ptr = (__m256*)stPtr + thread_num * thread_task;
	for (int i = 0; i < thread_leave_task; ++i, ++ptr) {
		_mm256_store_ps(retMax, _mm256_max_ps(*(__m256*)retMax, _mm256_log_ps(_mm256_sqrt_ps((*ptr)))));
	}

	//sse剩余任务，并存入第一个元素
	for (int i = 0; i < sse_leave_task; ++i) {
		float value = log(sqrt(data[stInd + len - 1 - i]));
		if (retMax[0] < value) {
			retMax[0] = value;
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
*/
DWORD WINAPI maxArray_multithread(LPVOID lpParameter)
{
	THREAD_DATA_SORT* threadData = (THREAD_DATA_SORT*)lpParameter;

	int end_index = threadData->start_index + threadData->block_data_len;
	float max_value = rawFloatData[threadData->start_index];
	for (int i = threadData->start_index; i < end_index; i++)
	{
		max_value = max_value >= rawFloatData[i] ? max_value : rawFloatData[i];//
	}
	floatResults[threadData->thread_id] = max_value;
	ReleaseSemaphore(hSemaphores[threadData->thread_id], 1, NULL);//释放信号量，信号量加1 

	return 0;
}


/*
* 求最大值加速版
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素中最大值
*/
float maxSpeedUp_multithread(const float data[], const int len) {
	int firstHalfLen; // client 处理前面一半数据，len 为数据长度
	enum Method mtd = Method::MT_MAX;//请求类型为max方法
	//0. 向Server端发送分布运算请求
	send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
	//1. 计算前面一半
	recv(cltConnection, (char*)&firstHalfLen, sizeof(int), NULL);
	//ret = myMax(data, firstHalfLen);
	//std::cout << "Current Client Max result = " << ret << std::endl;

	LARGE_INTEGER start_time, end_time, time_freq;
	QueryPerformanceFrequency(&time_freq);
	QueryPerformanceCounter(&start_time);//start  
	//================================================multi-thread
	THREAD_DATA_SORT* current_thread_data = new THREAD_DATA_SORT[MAX_THREADS];
	HANDLE hThreads[MAX_THREADS];
	// 1.1 create thread task
	for (int thread_index = 0; thread_index < MAX_THREADS; thread_index++)
	{
		hSemaphores[thread_index] = CreateSemaphore(NULL, 0, 1, NULL);
		// 线程输入数据
		current_thread_data[thread_index].thread_id = thread_index;
		current_thread_data[thread_index].start_index = 0;	// index in origin rawData
		current_thread_data[thread_index].block_data_len = CLIENT_SUBDATANUM;  // [start, end)
		floatResults[thread_index] = data[current_thread_data[thread_index].start_index];  // set default

		hThreads[thread_index] = CreateThread(
			NULL,// default security attributes
			0,// use default stack size
			maxArray_multithread,// thread function
			&current_thread_data[thread_index],// argument to thread function
			CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
			NULL);

		if (hThreads[thread_index] == NULL)
		{
			printf("WARN: hThreads=%d CreateThread error: %d\n", thread_index, GetLastError());
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
	float client_max_result = data[0];
	for (int i = 0; i < MAX_THREADS; i++)
		client_max_result = client_max_result > floatResults[i] ? client_max_result : floatResults[i];

	QueryPerformanceCounter(&end_time);//start 
	std::cout << "CLIENT Get Maxiumu finished! max_value=" << client_max_result <<
		", costs = " << ((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart << "s" << std::endl;

	//2. 把结果发送给Server端整合
	send(cltConnection, (char*)&client_max_result, sizeof(client_max_result), NULL);
	std::cout << "Send2Server success!" << std::endl;

	return ret;
}


/*
* 求最大值加速版
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素中最大值
*/
float maxSpeedUp(const float data[], const int len) {
	float ret;
	int ind;
	enum Method mtd = Method::MT_MAX;//请求类型为max方法
	//向Server端发送分布运算请求
	send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
	//获得任务分配
	recv(cltConnection, (char*)&ind, sizeof(int), NULL);
	//求解分任务
	ret = singleSpdMax(data, 0, ind + 1);
	//std::cout << "Current Client Max result = " << ret << std::endl;
	//把结果发送给Server端整合
	send(cltConnection, (char*)&ret, sizeof(ret), NULL);
	//std::cout << "Send2Server success!" << std::endl;

	////从Server端得到最终结果
	recv(cltConnection, (char*)&ret, sizeof(ret), NULL);


	return ret;
}

// =============================================== SORT
DWORD WINAPI sortArray_multithread(LPVOID lpParameter)
{
	THREAD_DATA_SORT* threadData = (THREAD_DATA_SORT*)lpParameter;
	//float* data = new float[threadData->block_data_len];

	int end_index = threadData->start_index + threadData->block_data_len;
	for (int new_index = 0, i = threadData->start_index; i < end_index; i++, new_index++) {
		ClientSortResult[threadData->thread_id][new_index] = rawFloatData[i];
	}
	quickSort(ClientSortResult[threadData->thread_id], 0, threadData->block_data_len - 1);  // void quickSort(float* data, int lowIndex, int highIndex) 

	ReleaseSemaphore(hSemaphores[threadData->thread_id], 1, NULL);//释放信号量，信号量加1 
	return 0;
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
	float** res = new float* [8];//SSE加速要分成256/32=8份
	for (int i = 0; i < 8; ++i) {
		res[i] = new float[len / 8];//每份要存len/8个元素排序后的数组
	}
	int sse_iter = len / 8;//SSE迭代总长度
	const float* stPtr = data + stInd;//偏移后的数组首地址

#pragma omp parallel for
	for (int i = 0; i < MAX_THREADS; ++i) {
		__m256* ptr = (__m256*)stPtr + i * sse_iter / MAX_THREADS;//这是SSE加速的分段首地址
		//SSE加速
		for (int j = 0; j < sse_iter / MAX_THREADS; ++j, ++ptr) {
			//mm256的浮点比较和浮点交换我都不知道用哪个才对。排序算法逻辑更是懵逼

		}
	}
	//结果整合
	//TODO:把结果整合进result参数


	//回收堆区内存
	for (int i = 0; i < 8; ++i) {
		delete[] res[i];
	}
	delete[] res;

	return 0.0f;
}

/*
* 双机排序
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data排序后的结果
*/
float sortSpeedUp(const float data[], const int len, float result[]) {
	int firstHalfLen; // 数据长度, client 处理前一部分数据，
	enum Method mtd = Method::MT_SORT;//请求类型为max方法
	//0. 向Server端发送分布运算请求
	send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
	//1. 获得任务分配
	recv(cltConnection, (char*)&firstHalfLen, sizeof(int), NULL);
	printf("CLIENT Receve SORT Task, Num=%d\n", firstHalfLen);

	//2. client 对前一半数据排序
	LARGE_INTEGER start_time, end_time, time_freq;
	QueryPerformanceFrequency(&time_freq);
	QueryPerformanceCounter(&start_time);

	THREAD_DATA_SORT* current_thread_data = new THREAD_DATA_SORT[MAX_THREADS]; // MAX_THREADS 个数据块
	HANDLE hThreads[MAX_THREADS];
	// 1.1 create thread task
	for (int thread_index = 0; thread_index < MAX_THREADS; thread_index++)
	{
		hSemaphores[thread_index] = CreateSemaphore(NULL, 0, 1, NULL);
		// 线程输入数据
		current_thread_data[thread_index].thread_id = thread_index;
		current_thread_data[thread_index].start_index = thread_index * CLIENT_SUBDATANUM;	// index in origin rawData
		current_thread_data[thread_index].block_data_len = CLIENT_SUBDATANUM;  // [start, end)

		hThreads[thread_index] = CreateThread(
			NULL,// default security attributes
			0,// use default stack size
			sortArray_multithread,// thread function
			&current_thread_data[thread_index],// argument to thread function
			CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
			NULL);

		if (hThreads[thread_index] == NULL)
		{
			printf("WARN: SORT hThreads=%d CreateThread error: %d\n", thread_index, GetLastError());
			throw std::exception("WARN: SORT CreateThread error!");
		}
	}
	// 1.2.execute + wait
	for (int i = 0; i < MAX_THREADS; i++)
	{
		ResumeThread(hThreads[i]);
	}
	std::cout << "Start WaitForMultipleObjects for 2mins!" << std::endl;
	DWORD ThreadFinished = WaitForMultipleObjects(MAX_THREADS, hSemaphores, TRUE, 2 * 60 * 1000);
	if (!((WAIT_OBJECT_0 <= ThreadFinished) && (ThreadFinished <= (WAIT_OBJECT_0 + MAX_THREADS - 1)))) {
		// exception
		printf("WARN: thread end exception! ThreadFinished=%d", ThreadFinished);
		throw std::exception("WARN: getMax thread end exception!");
	}
	std::cout << "Sort multihthread Finish!" << std::endl;

	for (int thread_index = 0; thread_index < MAX_THREADS; thread_index++) {
		printf("sortResultArraySequece No.%d/%d ,data_len=%d\n", thread_index, MAX_THREADS, CLIENT_DATANUM);
		for (int i = 0; i < current_thread_data[thread_index].block_data_len; i++) {
			std::cout << ClientSortResult[thread_index][i] << " ";
		}
		std::cout << std::endl;
	}


	// 1.3 merge server's result(Current in dumb ways: merge each 2 sequence)
	SORTED_ARRAY_MERGE* tmp_array_data = new SORTED_ARRAY_MERGE;
	SORTED_ARRAY_MERGE* merge_result_data = new SORTED_ARRAY_MERGE;
	tmp_array_data->merged_array = new float[CLIENT_DATANUM] {0};
	tmp_array_data->merged_len = 0;
	merge_result_data->merged_array = ClientSortMergedResult;
	merge_result_data->merged_len = 0;

	// 多个线程数据
	int current_merge_thread_index = 1;
	while (current_merge_thread_index < MAX_THREADS) {
		// initialize tmp_array >> merge (tmp_array, sortResultArraySequece[current_merge_array_no])
		if (current_merge_thread_index == 1) {
			int first_array_len = CLIENT_SUBDATANUM;
			merge_2_sorted_array(ClientSortResult[0], first_array_len, ClientSortResult[current_merge_thread_index], SERVER_SUBDATANUM, merge_result_data);
		}
		else {
			// merge from last time merge result
			for (int i = 0; i < merge_result_data->merged_len; i++) {
				tmp_array_data->merged_array[i] = merge_result_data->merged_array[i];
			}
			tmp_array_data->merged_len = merge_result_data->merged_len;

			merge_2_sorted_array(tmp_array_data->merged_array, tmp_array_data->merged_len,
				ClientSortResult[current_merge_thread_index], SERVER_SUBDATANUM, merge_result_data);
		}
		current_merge_thread_index++;
	}
	delete[] tmp_array_data->merged_array;
	delete tmp_array_data;

	int sort_success_flag = check_sorted_result(merge_result_data->merged_array, merge_result_data->merged_len);
	QueryPerformanceCounter(&end_time);//start 
	std::cout << "CLIENT SORT finished! sort_success=" << sort_success_flag <<
		", costs = " << ((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart << "s" << std::endl;
	for (int i = 0; i < min(CLIENT_DATANUM, 10); i++) {
		std::cout << ClientSortMergedResult[i] << " ";
	}
	std::cout << std::endl;

	//3. 把结果发送给Server端整合
	memset(buf, 0, BUF_LEN);
	printf("client Start waiting for server's request...\r\n");
	while ((recv_len = recv(cltConnection, buf, BUF_LEN, 0)) < 0) {}
	printf("Receve SendingResult Request from Server! msg=%s\n ", buf);

	float* client_sort_reuslt_pointer = ClientSortMergedResult;
	for (int i = 0; i < S_TIMES; i++)
	{
		if ((send_len = send(cltConnection, (char*)client_sort_reuslt_pointer, S_ONCE * sizeof(float), 0)) < 0)
		{
			printf("send result failed...\r\n");
			return -1;
		}
		//printf("%d, %f\r\n", send_len, *client_sort_reuslt_pointer);
		client_sort_reuslt_pointer += S_ONCE;
	}
	if (S_LEFT)
	{
		if ((send_len = send(cltConnection, (char*)client_sort_reuslt_pointer, S_LEFT * sizeof(float), 0)) < 0)
		{
			//printf("send result failed...\r\n");
			return -1;
		}
		printf("%d, %f\r\n", send_len, *client_sort_reuslt_pointer);

	}
	printf("Client sort result send successfully!\r\n");
	std::cout << "Send2Server success!" << std::endl;

	return 0.0f;
}


/*
* 测试用函数
*/
float test(float method(const float*, int), float data[], int len) {
	float ret;//返回值
	LARGE_INTEGER start, end;//存放测试算法的起始时间戳
	LARGE_INTEGER total;//总时间消耗
	total.QuadPart = 0;

	int testTurn = 5;//所需测试轮数
	for (int i = 1; i <= testTurn; ++i) {
		std::cout << "Test for " << i << " turn." << std::endl;
		//测试算法
		QueryPerformanceCounter(&start);//start  
		ret = method(data, len);
		QueryPerformanceCounter(&end);//end
		total.QuadPart += end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed is: " << (end.QuadPart - start.QuadPart) / 1000 << "ms." << std::endl;
		std::cout << "The result is: " << ret << std::endl;
	}
	std::cout << "Average time cost is " << total.QuadPart / 1000 / testTurn << " ms." << std::endl;
	return ret;
}

float test(float method(const float*, int, float*), float data[], int len, float result[])
{
	LARGE_INTEGER start, end;//存放测试算法的起始时间戳
	//测试算法
	QueryPerformanceCounter(&start);//start  
	method(data, len, result);
	QueryPerformanceCounter(&end);//end
	std::cout << "Time Consumed is: " << (end.QuadPart - start.QuadPart) << "ms." << std::endl;
	std::cout << "The result is " << ((result[0] < result[1]) ? "right." : "wrong.") << std::endl;
	return 0;
}

//主函数
int main() {
	using namespace std;

	//初始化
	//数据测试相关
	srand(RANDOM_SEED);
	for (size_t i = 0; i < DATANUM; i++) {//数据初始化
		rawFloatData[i] = fabs(double(rand()));  // float(i + 1);
		//rawFloatData[i] = float(i + 1);
	}
	for (size_t i = 0; i < min(DATANUM, 10); i++) {
		std::cout << rawFloatData[i] << " ";
	}
	std::cout << std::endl;

	float* result = new float[DATANUM];
	//流程控制
	int lpFlag;//循环控制Flag
	string cmd;//存储用户输入
	map<string, Method> cmdMap;//根据用户输入字符映射枚举
	cmdMap["sum"] = Method::MT_SUM;
	cmdMap["max"] = Method::MT_MAX;
	cmdMap["sort"] = Method::MT_SORT;

	//初始化网络连接
	initCltSocket();

	//测试循环
	lpFlag = 0;
	while (lpFlag == 0) {

		//要求用户输入需要测试的函数
		std::cout << "Please input the method you want test: " << endl;
		std::cout << "sum|max|sort" << endl;
		std::cin >> cmd;


		//从cmdMap中得到映射
		auto iter = cmdMap.find(cmd);
		if (iter == cmdMap.end()) {//检查是否有此命令
			std::cout << "Method " << cmd << " does't exist." << endl;
			continue;//没有就直接跳进下一个循环重新输入
		}
		enum Method mtd = iter->second;


		//根据命令映射调用不同的方法进行测试
		switch (mtd) {
		case Method::MT_SUM:
			std::cout << "Testing sum method..." << endl;
			test(SimpleSum, rawFloatData, DATANUM);
			std::cout << "Testing sumSpeedUp method..." << endl;
			test(sumSpeedUp, rawFloatData, DATANUM);
			break;
		case Method::MT_MAX:
			std::cout << "Testing max method..." << endl;
			test(myMax, rawFloatData, DATANUM);
			std::cout << "Testing maxSpeedUp method..." << endl;
			test(maxSpeedUp, rawFloatData, DATANUM);
			break;
		case Method::MT_SORT:
			//std::cout << "Testing sort method..." << endl;
			//test(sort, rawFloatData, DATANUM, result);
			std::cout << "Testing sortSpeedUp method..." << endl;
			test(sortSpeedUp, rawFloatData, DATANUM, result);
			break;
		default:
			break;
		}

		//询问用户是否继续测试
		std::cout << "Continue to test? " << endl;
		std::cout << "Input N to end/Input anything other to continue." << endl;
		std::cin >> cmd;
		if (cmd == "N") {
			lpFlag = -1;
			mtd = Method::END;
			send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
		}
		std::cout << endl;
	}


	//关闭网络连接
	closeSocket();

	return 0;

}


#pragma endregion
