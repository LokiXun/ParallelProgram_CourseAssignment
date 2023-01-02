//#include <sys/time.h>  // Linux
#include <stdint.h>
#include <Windows.h>
#include "MyMatrix.hpp"

#define DATAEXCHANGEDLL_EXPORTS
#ifdef DATAEXCHANGEDLL_EXPORTS
#define DATAEXCHANGEDLL_API __declspec(dllexport)
#else
#define DATAEXCHANGEDLL_API __declspec(dllimport)
#endif

#define CPREFIX extern "C"

int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

extern "C" {
	// init 2D int matrix
	DATAEXCHANGEDLL_API MyMatrix<int>* init_myMatrix2D_int(int** buff, int rowsNum, int columnsNum) {
		cout << "rowsNum, columnsNum=" << rowsNum << "," << columnsNum << endl;
		MyMatrix<int>* my_2d_matrix = new MyMatrix<int>(buff, rowsNum, columnsNum);
		cout << *my_2d_matrix << std::endl;
		return my_2d_matrix;
	}
	DATAEXCHANGEDLL_API MyMatrix<int>* multiply_2matrix(MyMatrix<int>* firstMatrix, MyMatrix<int>* secondMatrix) {
		std::cout << "multiply_2matrix==============" << endl;
		std::cout << "firstMatrix=\n" << *firstMatrix << endl;
		std::cout << "secondMatrix=\n" << *secondMatrix << endl;
		MyMatrix<int> tmpMatrix = firstMatrix->multiply(*secondMatrix);
		MyMatrix<int>* resultMatrix = new MyMatrix<int>(tmpMatrix);
		cout << "result = \n" << *resultMatrix;
		return resultMatrix;
	}
	DATAEXCHANGEDLL_API MyMatrix<int>* multiply_int(MyMatrix<int>* firstMatrix, int multiplyIntData) {
		std::cout << "multiply_int==============" << endl;
		std::cout << "firstMatrix=\n" << * firstMatrix << endl;
		std::cout << "multiplyIntData=\n" << multiplyIntData << endl;

		MyMatrix<int> tmpMatrix = firstMatrix->multiply(multiplyIntData);
		MyMatrix<int>* resultMatrix = new MyMatrix<int>(tmpMatrix);
		cout << "result = \n" << *resultMatrix;
		return resultMatrix;
	}
}

int main(int argc, char const* argv[])
{
	// 随机数种子
	srand(1);

	// 1. 无参构造
	MyMatrix<int> mat1;
	cout << "default constructor mat1 = \n"
		<< mat1 << endl;

	// 2. 方阵构造
	MyMatrix<float> mat2(3);
	cout << "eye-matrix constructor mat2 = \n"
		<< mat2 << endl;

	// 3. 一般矩阵构造
	MyMatrix<double> mat3(2, 3);
	cout << "2x3c matrix mat3 = \n"
		<< mat3 << endl;

	// 4. 把buffer转换成m*n的矩阵
	// [1, 2, 3;
	//  7, 8, 9]
	int** buffer;
	buffer = new int* [2];
	buffer[0] = new int[3];
	buffer[1] = new int[3];
	buffer[0][0] = 1;
	buffer[0][1] = 2;
	buffer[0][2] = 3;
	buffer[1][0] = 7;
	buffer[1][1] = 8;
	buffer[1][2] = 9;

	MyMatrix<int> mat4(buffer, 2, 3);
	cout << "buff构造 mat4 = \n"
		<< mat4 << endl;

	delete[] buffer[0];
	delete[] buffer[1];
	delete[] buffer;

	// 5. 复制构造
	MyMatrix<int> mat5(mat4);
	cout << "复制构造 mat5 := mat4 = \n"
		<< mat5 << endl;

	// 6. 加减乘法（数）
	cout << "mat4 + 66 = \n"
		<< mat4 + 66 << endl;
	cout << " 66 + mat4= \n"
		<< 66 + mat4 << endl;
	mat4 += 66;
	cout << "mat4 += 66, mat4 = \n"
		<< mat4 << endl;

	cout << "mat4 - 66 = \n"
		<< mat4 - 66 << endl;
	mat4 -= 66;
	cout << "mat4 -= 66, mat4 = \n"
		<< mat4 << endl;

	cout << "mat4 * 66 = \n"
		<< mat4 * 66 << endl;
	cout << "66 * mat4 = \n"
		<< 66 * mat4 << endl;
	mat4 *= 66;
	cout << "mat4 *= 66, mat4 = \n"
		<< mat4 << endl;

	// 7. 加减乘法（矩阵）
	cout << "mat4 = mat5 = \n"
		<< (mat4 = mat5) << endl;
	cout << "mat4 + mat5 = \n"
		<< mat4 + mat5 << endl;
	mat4 += mat5;
	cout << "mat4 += mat5, mat4 = \n"
		<< mat4 << endl;

	cout << "mat4 - mat5 = \n"
		<< mat4 - mat5 << endl;
	mat4 -= mat5;
	cout << "mat4 -= mat5, mat4 = \n"
		<< mat4 << endl;

	cout << "mat4 * mat5.T = \n"
		<< mat4 * mat5.transpose() << endl;
	mat4 *= mat5.transpose();
	cout << "mat4 *= mat5.T, mat4 = \n"
		<< mat4 << endl;

	// 8. 获取成员
	cout << "mat4.row = " << mat4.getRowsNum() << ", mat4.col = " << mat4.getColsNum() << endl;
	cout << "get mat4(0, 1) = " << mat4(0, 1) << endl;
	cout << "get mat4[0][1] = " << mat4[0][1] << endl;

	// 9. 修改成员
	mat4(0, 1) = 666;
	cout << "set mat4(0, 1) = 666, mat4 = \n"
		<< mat4 << endl;

	// 10. 求逆
	MyMatrix<double> mat6(3, 3);
	mat6(0, 2) = 1.0;
	mat6(1, 0) = 1.0;
	mat6(2, 1) = 1.0;
	cout << "mat6 = \n"
		<< mat6 << endl;
	cout << "mat6.inverse = \n"
		<< mat6.inverse() << endl;
	cout << "mat6 * mat6.inverse = \n"
		<< mat6 * mat6.inverse() << endl;

	mat6 = MyMatrix<double>::random(3, 3);
	cout << "mat6 = \n"
		<< mat6 << endl;
	cout << "mat6.inverse = \n"
		<< mat6.inverse() << endl;
	cout << "mat6 * mat6.inverse = \n"
		<< mat6 * mat6.inverse() << endl;

	// 11. 乘法加速
	int size = 20;
	MyMatrix<int> a = MyMatrix<int>::random(size, size, 10);
	MyMatrix<int> b = MyMatrix<int>::random(size, size, 10);
	MyMatrix<int> c;

	// 计时相关
	struct timeval startv, endv;
	//struct timezone startz, endz;
	// 时间间隔，单位us
	long t_usec_base = 0;
	long t_usec_improved = 0;

	cout << "Calculating...(about 60s on my machine, size=2000)" << endl;

	// normal
	gettimeofday(&startv, NULL);
	c = a.multiply(b);
	// cout << c << endl;
	gettimeofday(&endv, NULL);
	t_usec_base = (endv.tv_sec - startv.tv_sec) * 1000000 + (endv.tv_usec - startv.tv_usec);
	printf("Method: duration = %ld us\n", t_usec_base);

	system("pause");
	return 0;
}
