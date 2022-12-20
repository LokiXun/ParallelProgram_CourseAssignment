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
	//待测试数据
	float* rawFloatData = new float[DATANUM];
	for (size_t i = 0; i < DATANUM; i++) {//数据初始化
		rawFloatData[i] = float(i + 1);
	}
	float* result = new float[DATANUM] {0};//存放排序过的float数组
	float ret = 0.0f;
	//循环控制
	int lpFlag;


	//初始化网络连接
	initSrvSocket();


	//测试循环

	lpFlag = 0;
	enum Method mtd = Method::WAIT;
	while (lpFlag == 0) {
		//TODO:总之这部分仅仅是实现了功能，对于连接中断后的处理，连接的多线程化以及线程池管理等都没有做。
		//问题就是多线程我不熟悉，运行逻辑也不清楚
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
			sortSpeedUp(rawFloatData, DATANUM, result);
			//排序检验的代码随便写的
			cout << "The result is " << ((result[0] < result[1]) ? "right." : "wrong.") << endl;
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
	float* retSum = new float[2]{ 0 };
	int ind = int(len / 2) -1;//ind指client段所需计算到的下标

	// 0. 任务分配
	// 目前只有2台机器，发送前一半给client计算，后一半自己算
	send(servConnection, (char*)&ind, sizeof(int), NULL);

	//1. server 算计算另一半 TODO: 调用client里面的同一个函数
	retSum[0] = SumArray_speedUp(data, ind, len-1);
	
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
* 求最大值加速版（单机版）
* @data 待求和的float一维数组
* @len float数组长度
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


/**
* 加速版求最大值
* @data 待求最大元素的float一维数组
* @len float数组长度
* @return 数组data各元素中的最大值
*/
float maxSpeedUp(const float data[], const int len) {
	float ret;
	//TODO:本来想搞更灵活的调度方式的，但是多线程没搞懂
	float* retMax = new float[2]{ 0 };
	int ind = len / 2;//ind指client段所需计算到的下标
	send(servConnection, (char*)&ind, sizeof(int), NULL);//发送运算任务
	//然后server端自己算
	retMax[0] = singleSpdMax(data, ind, len - ind);
	//接收Client端的结果
	recv(servConnection, (char*)&retMax[1], sizeof(float), NULL);
	//整合结果
	if (retMax[0] > retMax[1]) {
		ret = retMax[0];
	}
	else {
		ret = retMax[1];
	}
	//把最终结果返回Client端
	send(servConnection, (char*)&ret, sizeof(ret), NULL);

	return ret;
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
	float* res = new float[8]{ 0 };//SSE加速的中间
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

/**
* 加速版排序
* @data 待排序的float一维数组
* @len float数组长度
* @return 若正常求解，则返回0.0
*/
float sortSpeedUp(const float data[], const int len, float result[]) {

	int ind = len / 2;//ind指client段所需计算到的长度（下标+1）
	float** retSort = new float* [2]{ new float[ind],new float[ind] };
	//0. 发送运算任务给 client
	send(servConnection, (char*)&ind, sizeof(int), NULL);
	//然后server端自己算
	singleSpdSort(data, ind, len - ind, retSort[0]);
	//接收Client端的结果
	recv(servConnection, (char*)&retSort[1], ind * sizeof(float), NULL);
	
	// 整合结果
	system("pause");

	//把最终结果返回Client端
	send(servConnection, (char*)&result, ind * sizeof(float), NULL);

	return 0.0;
}


#pragma endregion