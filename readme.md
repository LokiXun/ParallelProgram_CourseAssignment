# Parallel Programming Course Assignment :baby_chick:

Team: RongjiXun, JianLiangChen



﻿﻿﻿﻿﻿﻿﻿﻿﻿## 作业1、二维矩阵库编写

要求：
    a、支持**不同大小的矩阵**，并可以放入**不同的数据类型** 》》模板
	b、构造方法：(1)默认构造；**(2)方阵构造；(3)一般矩阵构造**； (4)复制构造；
	c、**运算符重载**：(1)加法 + ；(2)减法 - ；(3)矩阵数乘、矩阵相乘 * ；(4)复制赋值 ；(5)输出流运算符 << 
	d、其他方法：(1)获取成员；(2)修改成员；(3)动态内存分配
评分标准，功能实现60分；实现**矩阵求逆和伪逆**且考虑异常情况的，矩阵乘法写普通版版本和**加速版本**的，且加速比大于2.0的10分；
在python环境中调用自己写的程序，并和python直接实现以及调用numpy实现矩阵乘法三者进行比较和分析30分（不能直接使用时，可以借助于cython 最终实现最终在python的调用）；提交源代码和工程文件（每人一份），提交的压缩包大于5MB的扣5分。

矩阵类名: MyMatrix，支持一下使用方法

```cpp
MyMatrix<int> temp;
MyMatrix<double> temp(m,n)）;//构造m*n的空矩阵
MyMatrix<int> temp(buffer,m,n);//把buffer转换成m*n矩阵

MyMatrix<int> temp2(temp);
MyMatrix<int> temp3;
temp3 = temp;//所有元素都要拷贝过去，深拷贝

temp=temp+temp2;//同样大小的矩阵
temp2=temp*3;//每个元素乘以3
temp2=temp-3;//每个元素加3
temp*=temp3;//支持矩阵乘法和矩阵向量乘，如果不能乘返回值

cout<<temp<<endl;//输出temp格式到屏幕
```

- `CustomMatrix`

> [参考](https://medium.com/@furkanicus/how-to-create-a-matrix-class-using-c-3641f37809c7)
> [参考2](http://www.quantstart.com/articles/Matrix-Classes-in-C-The-Header-File/)





## 大作业

内容：两人一组，利用相关**C++和加速(sse，多线程)手段，以及通讯技术(1.rpc，命名管道，2.http，socket)**等实现函数（浮点数**数组求和，求最大值，排序**）。处理在两台计算机**协作**执行，尽可能挖掘两个计算机的潜在算力。所编写功能在 **python 环境中调用**，并和 python 实现的该功能的代码进行性能比较。

要求：书写完整报告，给出设计思路和结果。特别备注我重现你们代码时，需要修改的位置和含义，精确到文件的具体行。

提交材料：报告和程序。

- 给分细则

  功能实现且实现 python 的调用80分；
  **非windows上实现**10分（操作系统限于ubuntu,android）；
  **显卡加速**者且加速比大于2.0的10分。
  提交源代码、工程文件和报告（一组一份），提交的压缩包大于`10MB`的扣5分。加速比越大分数越高；多人组队的，分数低于同加速比的两人组分数；

> 备注：报告中列出执行5次任务，并求时间的平均值。需要附上两台单机原始算法执行时间，以及利用双机并行执行同样任务的时间。
>
> 特别说明：最后加速比以我测试为准。报告中的时间统计必须真实有效，发现舞弊者扣分。如果利用超出我列举的技术和平台，先和我商议。
>
> 追加：**三个功能自己代码实现，不得调用第三方库函数**（比如，sd::max,std::sort）,违反者每函数扣10分。多线程，多进程，OPENMP可以使用。



数据：自己生成，可参照下述方法。

测试数据量和计算内容为：

```cpp
#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM * MAX_THREADS)   /*这个数值是总数据量*/

// 待测试数据定义为：
float rawFloatData[DATANUM];

// 参照下列数据初始化代码：**两台计算机可以分开初始化，减少传输代价**
for (size_t i = 0; i < DATANUM; i++)//数据初始化
{
	rawFloatData[i] = float(i+1);
}
```

为了模拟任务：每次访问数据时，用`log(sqrt(rawFloatData[i]))`进行访问！就是说比如计算加法 用 `sum+=log(sqrt(rawFloatData[i]))`,而不是 `sum+=rawFloatData[i]` !!。这里计算结果和存储精度之间有损失，但你们机器的指令集限制，**如果使用`SSE`中的**`double`型的话，单指令只能处理4个double，**如果是float则可以8个。所以用float加速比会更大。**

- **提供代码（1/2）**
  需要提供的函数：(不加速版本，为同样的数据量在两台计算机上各自运行的时间。算法一致，只是**不采用任何加速手段（SSE，多线程或者OPENMP)）**

  ```cpp
  float sum(const float data[],const int len); //data是原始数据，len为长度。结果通过函数返回
  float max((const float data[],const int len);//data是原始数据，len为长度。结果通过函数返回
  float sort((const float data[],const int len, float  result[]);//data是原始数据，len为长度。排序结果在result中。
  ```

- **提供代码（2/2）双机加速版本**

  ```cpp
  float sumSpeedUp(const float data[],const int len); //data是原始数据，len为长度。结果通过函数返回
  float maxSpeedUp(const float data[],const int len);//data是原始数据，len为长度。结果通过函数返回
  float sortSpeedUp(const float data[],const int len, float result[]);//data是原始数据，len为长度。排序结果在result中。
  ```

  加速中如果使用SSE，特别注意**SSE的指令和数据长度有关**，单精度后缀 ps ,双精度后缀 pd。

  

- **测试速度的代码框架为**

  ```cpp
  QueryPerformanceCounter(&start);//start  
  你的函数(...);//包括任务启动，结果回传，收集和综合
  QueryPerformanceCounter(&end);//end
  std::cout << "Time Consumed:" << (end.QuadPart - start.QuadPart) << endl;
  cout<<输出求和结果<<endl;
  cout<<输出最大值<<endl;
  cout<<输出排序是否正确<<endl;
  ```

  

大家注意，如果单机上那么大数据量无法计算，要想办法**可能会遇到超大数加很小数加不上的现象**。修改 `#define SUBDATANUM 2000000` 为 `#define SUBDATANUM 1000000`做单机计算。双机上每个计算机都申请`#define SUBDATANUM 1000000`大的数据，即实现`#define SUBDATANUM 2000000`的运算。



- Max
  $$
  argmin_x (y = x + \frac{Num}{x} )\\
  y = (\sqrt{x})^2 +- 2\sqrt{C}*\sqrt{x} * \frac{1}{\sqrt{x}} + \frac{C}{(\sqrt{x})^2} \\
  y = (\sqrt{x} - \frac{\sqrt{C}}{\sqrt{x}})^2 + 2*\sqrt{C} \\
  
  \therefore \text{the computation time y is minimum when } x=\sqrt{C}, \\
  \text{ has x workers>>} \sqrt{C} \quad \\\text{ to compute max element their own part of array in parallel.}
  $$
  
