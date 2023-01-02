﻿﻿﻿﻿﻿﻿﻿﻿## 作业1、二维矩阵库编写

Author: RongjiXun

要求：
    a、支持**不同大小的矩阵**，并可以放入**不同的数据类型** 》》模板
	b、构造方法：(1)默认构造；**(2)方阵构造；(3)一般矩阵构造**； (4)复制构造；
	c、**运算符重载**：(1)加法 + ；(2)减法 - ；(3)矩阵数乘、矩阵相乘 * ；(4)复制赋值 ；(5)输出流运算符 << 
	d、其他方法：(1)获取成员；(2)修改成员；(3)动态内存分配
评分标准，功能实现60分；实现**矩阵求逆和伪逆**且考虑异常情况的，矩阵乘法写普通版版本和**加速版本**的，且加速比大于2.0的10分；
**在python环境中调用自己写的程序，并和python直接实现以及调用numpy实现矩阵乘法三者进行比较和分析30分（不能直接使用时，可以借助于cython 最终实现最终在python的调用）**；提交源代码和工程文件（每人一份），提交的压缩包大于5MB的扣5分。

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

> [参考](https://medium.com/@furkanicus/how-to-create-a-matrix-class-using-c-3641f37809c7)
> [参考2](http://www.quantstart.com/articles/Matrix-Classes-in-C-The-Header-File/)

- Overload `<<` Operator

  > [参考](https://learn.microsoft.com/en-us/cpp/standard-library/overloading-the-output-operator-for-your-own-classes?view=msvc-170)

  The overloaded operator returns a reference to the original `ostream` object, which means you can combine insertions:

  ```cpp
  friend ostream& operator<<(ostream& out, const MyMatrix<T>& matrix)		//输出流运算符 << 
  {
      out << "[";
      for (int i = 0; i < matrix.m_rows; i++)
      {
          if (i != 0)
              out << " ";
          for (int j = 0; j < matrix.m_cols; j++)
          {
              out << matrix.m_data[i][j];
              if (i != matrix.m_rows - 1 || j != matrix.m_cols - 1)
                  out << ", ";
          }
          if (i != matrix.m_rows - 1)
              out << endl;
      }
      out << "]" << endl;
  
      return out;
  }
  ```

- overload double subcripts

  > [参考](https://www.youtube.com/watch?v=zVQutx4cdrM)



### ctypes import cpp

测试：对比myMatrix、numpy 矩阵乘法：数乘、矩阵乘

```python
np.random.seed(1)
matrix_a = np.random.randint(1, 100, size=(100, 100))
matrix_b = np.random.randint(1, 100, size=(100, 100))

my_2d_matrix_a = My2dMatrix(matrix_a)
my_2d_matrix_b = My2dMatrix(matrix_b)
my_2d_matrix_result = My2dMatrix(np.array([[]]))

```

- 数乘

  ```python
  multiply_int_data = 999
  my_2d_matrix_result.matrix_obj = My2dMatrix.multiply_with_int(
      my_2d_matrix_a.matrix_obj, multiply_int_data)  # 0.0181378s
  # My2dMatrix.multiply_with_int(my_2d_matrix_result.matrix_obj, 4)
  start_time = time.time_ns()
  result_matrix = matrix_a * multiply_int_data  # 0.0000000000s
  print(f"numpy multiply_with_int costs={(time.time_ns() - start_time) / (10 ** 9):.10f}s")
  ```

- 矩阵乘法

  ```python
  my_2d_matrix_a.multiply_with_matrix(my_2d_matrix_a.matrix_obj, my_2d_matrix_b.matrix_obj)  # 0.0039863s
  start_time = time.time_ns()
  result_matrix = matrix_a @ matrix_b
  print(f"numpy multiply_with_matrix costs={(time.time_ns() - start_time) / (10 ** 9):.10f}s")  # 0.0009999000s
  print(f"result_matrix\n={result_matrix}")
  ```

  

