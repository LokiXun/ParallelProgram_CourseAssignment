/*
* Client端主程序
*/

#define CLIENT
#define _WINSOCK_DEPRECATED_NO_WARNINGS//因为用到了过时方法inet_addr()，没这个过不了编译


#pragma region header
#include "ServerClientConfig.h"//pch为预编译头文件
#pragma endregion header



#pragma region global_variable
//SSE加速的方法共享同一网络连接
//我想通过连接当参数传入来实现封装，但是老师把函数参数定死了，就只能用全局变量了

WSAData wsaData;//socket库信息
WORD dllVersion;//dllVersion信息
SOCKADDR_IN addr;//网络地址信息
int addrlen = sizeof(addr);//addr数据长度
SOCKET cltConnection;//cltConnection为服务端连接
enum class Method {//用来分情况切换所测试的函数方法
	MT_SUM = 1,
	MT_MAX = 2,
	MT_SORT = 3,
	END = 9
};

#pragma endregion



#pragma region method_declaraion
//单机版
//不采取任何加速手段
float sum(const float data[], const int len); //data是原始数据，len为长度。结果通过函数返回
float myMax(const float data[], const int len);//data是原始数据，len为长度。结果通过函数返回
float sort(const float data[], const int len, float  result[]);//data是原始数据，len为长度。排序结果在result中。

//双机加速版本
//采取多线程和SSE技术加速
int initCltSocket();//初始化Socket
int closeSocket();//关闭Socket
float sumSpeedUp(const float data[], const int len); //data是原始数据，len为长度。结果通过函数返回
float maxSpeedUp(const float data[], const int len);//data是原始数据，len为长度。结果通过函数返回
float sortSpeedUp(const float data[], const int len, float result[]);//data是原始数据，len为长度。排序结果在result中。
void quickSort(float* data, int lowIndex, int highIndex);

//用于测试的函数
float test(float method(const float*, int), float data[], int len);
float test(float method(const float*, int, float*), float data[], int len, float result[]);

#pragma endregion



//主函数
int main() {
	using namespace std;

	//初始化
	//数据测试相关
	srand(RANDOM_SEED);
	for (size_t i = 0; i < DATANUM; i++) {//数据初始化
		rawFloatData[i] = fabs(double(rand()));  // float(i + 1);
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
			test(sum, rawFloatData, DATANUM);
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
			std::cout << "Testing sort method..." << endl;
			test(sort, rawFloatData, DATANUM, result);
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

	//回收动态数组内存


}


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
float sum(const float data[], const int len) {
	float* retSum = new float[MAX_THREADS] {0};
	int subDataNum = int(ceil(len / MAX_THREADS));//为了增加数值稳定性，进一步细分求解区域（然而还是有误差）
	float sumResult = 0.0f;
	//分块求和
	for (int i = 0; i < MAX_THREADS; ++i) {
		int subDataStartIndex = i * subDataNum;
		int SubDataElementNum = (i + 1 == MAX_THREADS) ? (len - subDataStartIndex) : subDataNum; // 判断此块元素个数

		for (int j = 0; j < SubDataElementNum; ++j) {
			//retSum[i] += log(sqrt(data[j + subDataStartIndex]));  // 模拟任务，增加计算量
			retSum[i] += data[j + subDataStartIndex];
		}
	}

	//整合结果
	for (int i = 0; i < MAX_THREADS; ++i) {
		sumResult += retSum[i];
	}

	return sumResult;
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


/*
=================== SORT =====================
*/

void quickSort(float* data, int lowIndex, int highIndex) {
	int i = lowIndex, j = highIndex;
	double x = data[i];

	while (i < j) {
		while (i < j and x <= data[j]) { j--; };
		if (i < j) {
			data[i++] = data[j];
		}
		while (i < j and data[i] < data[j]) { i++; };
		if (i < j) {
			data[j--] = data[i];
		}
	}
	data[i] = x;

	if (lowIndex < i - 1) { quickSort(data, 0, i - 1); }
	if (highIndex > (i + 1)) { quickSort(data, i + 1, highIndex); }
}

/*
* sort函数采用快排
* 问题是快排写的可能有问题
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


// ================================SUM
/*
* Sum : Speed Up Using SSE + OpenMP
* Func 对数据部分范围 [startIndex，endIndex] 闭区间, 求和。按 8 个数分块，使用SSE 256bit，最后一块不足 8 个对的部分就直接加
* @data 待求和的float一维数组
* @stInd
* @len float数组长度
* @return 数组data全元素的和
*/
float SumArray_speedUp(const float data[], const int startIndex, const int endIndex) {
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
* 加速版求和
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data全元素的和
*/
float sumSpeedUp(const float data[], const int len) {
	float ret;
	int ind; // client 处理前面一半数据，len 为数据长度
	enum Method mtd = Method::MT_SUM;
	//向Server端发送分布运算请求
	send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
	//获得任务分配: 前半段数组的结尾下标
	recv(cltConnection, (char*)&ind, sizeof(int), NULL);
	//求解分任务
	ret = SumArray_speedUp(data, 0, ind);
	std::cout << "Current Client Sum result = " << ret << std::endl;
	//把结果发送给Server端整合
	send(cltConnection, (char*)&ret, sizeof(ret), NULL);
	std::cout << "Send2Server success!" << std::endl;
	////从Server端得到最终结果
	//recv(cltConnection, (char*)&ret, sizeof(ret), NULL);

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
	float* retMax = new float[8] { 0 };//用于SSE加速的中间存储
	int sse_iter = len / 8;//SSE迭代总长度
	const float* stPtr = data + stInd;//偏移后的数组首地址
	//最大值赋初值
	_mm256_store_ps(retMax, _mm256_log_ps(_mm256_sqrt_ps(*(__m256*)stPtr)));

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
	ret = myMax(data, ind + 1);
	std::cout << "Current Client Max result = " << ret << std::endl;
	//把结果发送给Server端整合
	send(cltConnection, (char*)&ret, sizeof(ret), NULL);
	std::cout << "Send2Server success!" << std::endl;

	////从Server端得到最终结果
	//recv(cltConnection, (char*)&ret, sizeof(ret), NULL);

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

boolean checkSortedArray(const float data[], const int len) {

	for (int i = 0; i < len - 1; i++) {
		if (data[i] > data[i + 1]) {
			return FALSE;
		}
	}
	return TRUE;
}

/*
* 排序算法加速版
* @data 待求和的float一维数组
* @len float数组长度
* @return 数组data排序后的结果
*/
float sortSpeedUp(const float data[], const int len, float result[]) {
	float ret;
	int ind;
	enum Method mtd = Method::MT_SORT;//请求类型为max方法
	//0. 向Server端发送分布运算请求
	send(cltConnection, (char*)&mtd, sizeof(Method), NULL);
	//1. 获得任务分配
	recv(cltConnection, (char*)&ind, sizeof(int), NULL);

	//2. 排序
	sort(data, len, result);
	std::cout << "Client sort result=";
	for (int i = 0; i < len; i++) {
		std::cout << data[i] << " ";
	}
	std::cout << std::endl;
	// check
	checkSortedArray(data, len);

	//3. 把结果发送给Server端整合
	/*
	* TIPS: TCP在单机网上一次最大传输65536字节，即16384个float, 8192个double
	* 在局域网内根据网卡，一次最大传输1500字节，即375个float, 187个double
	*/

	std::cout << "Send2Server success!" << std::endl;

	return 0.0f;
}


/*
* 测试用函数
*/
float test(float method(const float*, int), float data[], int len) {
	LARGE_INTEGER start, end;//存放测试算法的起始时间戳
	//测试算法
	QueryPerformanceCounter(&start);//start  
	float ret = method(data, len);
	QueryPerformanceCounter(&end);//end
	std::cout << "Time Consumed is: " << (end.QuadPart - start.QuadPart) << "ms." << std::endl;
	std::cout << "The result is: " << ret << std::endl;
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




#pragma endregion
