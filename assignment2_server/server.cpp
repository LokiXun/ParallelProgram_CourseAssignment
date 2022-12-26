/*
* Server端主程序
*/

#define SERVER
#define _WINSOCK_DEPRECATED_NO_WARNINGS//因为用到了过时方法inet_addr()，没这个过不了编译
#include"ServerClientConfig.h"//pch为预编译头文件


#pragma region global_variable
WSAData wsaData;//socket库信息
WORD dllVersion;//dllVersion信息
SOCKADDR_IN addr;//网络地址信息
int addrlen = sizeof(addr);//addr数据长度
SOCKET sListen;
SOCKET servConnection;//servConnection为服务端连接
int ret, recv_len, send_len, sin_size; // 一些网络收发相关变量
HANDLE hSemaphores[MAX_THREADS];//信号量，保证不重入。等同于mutex】

#pragma endregion


#pragma region method_defination
// 初始化网络连接
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

//回收SSE资源
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
* Sum 双机版本：server 分发一半数据给 client，之后在 server 整合数据
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

	//1. server 算计算后一半
	retSum[0] = SumArray_speedUp(data, ind + 1, len - 1);

	//2. 接收Client端的结果
	recv(servConnection, (char*)&retSum[1], sizeof(float), NULL);

	//3. 整合结果
	ret = retSum[0] + retSum[1];
	std::cout << "Sum SpeedUp Calculate Finished! reesult = " << ret << std::endl;

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
	//================================================multi-thread
	int waited_array_start_index = int(ind) + 1;
	int CURRENT_THREAD_NUM = min(MAX_THREADS, len - waited_array_start_index); // 当前线程数，如果数据量 < MAX_THREADS 不需要开这么多
	int each_thread_process_num = ceil((len - waited_array_start_index) / CURRENT_THREAD_NUM);

	THREAD_DATA* current_thread_data = new THREAD_DATA[MAX_THREADS];
	HANDLE hThreads[MAX_THREADS];
	// 1.1 create thread task
	for (int i = 0; i < MAX_THREADS; i++)
	{
		hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);
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
/*
* 排序 multithread
*/
DWORD WINAPI sortArray_multithread(LPVOID lpParameter)
{
	THREAD_DATA_SORT* threadData = (THREAD_DATA_SORT*)lpParameter;
	//float* data = new float[threadData->block_data_len];

	int end_index = threadData->start_index + threadData->block_data_len;
	for (int new_index = 0, i = threadData->start_index; i < end_index; i++, new_index++) {
		ServerSortResult[threadData->thread_id][new_index] = rawFloatData[i];
	}
	quickSort(ServerSortResult[threadData->thread_id], 0, threadData->block_data_len - 1);  // void quickSort(float* data, int lowIndex, int highIndex) 

	ReleaseSemaphore(hSemaphores[threadData->thread_id], 1, NULL);//释放信号量，信号量加1 
	return 0;
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

	int firstHalfLen = MAX_THREADS * CLIENT_SUBDATANUM; // client 计算前面一部分数据长度
	int secondHalfLen = MAX_THREADS * SERVER_SUBDATANUM;
	
	// 0. 任务分配
	send(servConnection, (char*)&firstHalfLen, sizeof(int), NULL);
	printf("Send SortTask to CLIENT, firstHalfLen=%d\n", firstHalfLen);

	//1. server 对后一半排序
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
		current_thread_data[thread_index].start_index = firstHalfLen + thread_index * SERVER_SUBDATANUM;	// index in origin rawData
		current_thread_data[thread_index].block_data_len = SERVER_SUBDATANUM;  // [start, end)

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
		printf("sortResultArraySequece No.%d/%d ,data_len=%d\n", thread_index, MAX_THREADS, SERVER_DATANUM);
		for (int i = 0; i < current_thread_data[thread_index].block_data_len; i++) {
			std::cout << ServerSortResult[thread_index][i] << " ";
		}
		std::cout << std::endl;
	}


	// 1.3 merge server's result(Current in dumb ways: merge each 2 sequence)
	SORTED_ARRAY_MERGE* tmp_array_data = new SORTED_ARRAY_MERGE;
	SORTED_ARRAY_MERGE* merge_result_data = new SORTED_ARRAY_MERGE;
	tmp_array_data->merged_array = new float[SERVER_DATANUM] {0};
	tmp_array_data->merged_len = 0;
	merge_result_data->merged_array = ServerSortMergedResult;
	merge_result_data->merged_len = 0;

	// 多个线程数据
	int current_merge_thread_index = 1;
	while (current_merge_thread_index < MAX_THREADS) {
		// initialize tmp_array >> merge (tmp_array, sortResultArraySequece[current_merge_array_no])
		if (current_merge_thread_index == 1) {
			int first_array_len = SERVER_SUBDATANUM;
			merge_2_sorted_array(ServerSortResult[0], first_array_len, ServerSortResult[current_merge_thread_index], SERVER_SUBDATANUM, merge_result_data);
		}
		else {
			// merge from last time merge result
			for (int i = 0; i < merge_result_data->merged_len; i++) {
				tmp_array_data->merged_array[i] = merge_result_data->merged_array[i];
			}
			tmp_array_data->merged_len = merge_result_data->merged_len;

			merge_2_sorted_array(tmp_array_data->merged_array, tmp_array_data->merged_len,
				ServerSortResult[current_merge_thread_index], SERVER_SUBDATANUM, merge_result_data);
		}
		current_merge_thread_index++;
	}
	delete[] tmp_array_data->merged_array;
	delete tmp_array_data;

	int sort_success_flag = check_sorted_result(merge_result_data->merged_array, merge_result_data->merged_len);
	QueryPerformanceCounter(&end_time);//start 
	std::cout << "Server SORT finished! sort_success=" << sort_success_flag <<
		", costs = " << ((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart << "s" << std::endl;
	for (int i = 0; i < min(SERVER_DATANUM,10); i++) {
		std::cout << ServerSortMergedResult[i] << " ";
	}
	std::cout << std::endl;

	//2. 接收client端的结果
	// 2.1 给 client 发请求，要数据
	memset(buf, 0, BUF_LEN);
	sprintf_s(buf, "Come on!");
	while ((send_len = send(servConnection, buf, strlen(buf), 0)) < 0) {
		printf("server send getSortResult request failed...sleep for 10s...\r\n");
		Sleep(1 * 1000); // sleep 1s
	}
	printf("# Send: Come on!\r\n");

	// 2.2 每 1k 收数据
	float* clinet_sort_result = ClientSortMergedResult;
	for (int i = 0; i < S_TIMES; i++)
	{
		if ((recv_len = recv(servConnection, (char*)clinet_sort_result, S_ONCE * sizeof(float), 0)) < 0)
		{
			printf("receive client result failed...\r\n");
			return -1;
		}
		//printf("%d, %f\r\n", recv_len, *clinet_sort_result);
		clinet_sort_result += S_ONCE;
	}
	// process remained data
	if (S_LEFT)
	{
		if ((recv_len = recv(servConnection, (char*)clinet_sort_result, S_LEFT * sizeof(float), 0)) < 0)
		{
			printf("receive result failed...\r\n");
			return -1;
		}
		//printf("%d, %f\r\n", recv_len, *clinet_sort_result);
	}
	printf("CLIENT RECEVIED RESULT:");
	for (int i = 0; i < min(CLIENT_DATANUM,10); i++) {
		std::cout << ClientSortMergedResult[i] << " ";
	}
	std::cout << std::endl;


	//3. server & client 整合结果
	SORTED_ARRAY_MERGE* merge_total_result_server_client = new SORTED_ARRAY_MERGE;
	merge_total_result_server_client->merged_array = result;
	merge_total_result_server_client->merged_len = 0;
	merge_2_sorted_array(ClientSortMergedResult, CLIENT_DATANUM, ServerSortMergedResult, SERVER_DATANUM, merge_total_result_server_client);

	QueryPerformanceCounter(&end_time);//start 
	sort_success_flag = check_sorted_result(merge_total_result_server_client->merged_array, merge_total_result_server_client->merged_len);
	printf("Multi-PC-multi-thread[SORT](Server): answer = %d, time = %f us\r\n", sort_success_flag, ((double)end_time.QuadPart - start_time.QuadPart) / (double)time_freq.QuadPart);
	for (int i = 0; i < min(DATANUM, 10); i++) {
		std::cout << result[i] << " ";
	}
	std::cout << std::endl;
	return 0.0;
}



//主函数
int main() {
	using namespace std;
	long t_usec = 0;  // 时间间隔，单位us

	//初始化
	srand(RANDOM_SEED);
	for (size_t i = 0; i < DATANUM; i++) {//数据初始化
		rawFloatData[i] = fabs(float(rand()));  // float(i + 1);
	}
	float ret = 0.0f;

	for (size_t i = 0; i < min(DATANUM, 30); i++) {
		std::cout << rawFloatData[i] << " ";
	}
	std::cout << std::endl;

	//初始化网络连接
	initSrvSocket();

	//测试循环
	int lpFlag = 0;
	enum Method mtd = Method::WAIT;
	while (lpFlag == 0) {
		if ((recv_len = recv(servConnection, (char*)&mtd, sizeof(int), NULL)) < 0)	//接收指令
		{
			printf("receive result failed...\r\n");
			return -1;
		}
		//cout << "Client message received.";
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
			sortSpeedUp(rawFloatData, DATANUM, SortTotalResult);
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

#pragma endregion