# encoding: utf-8
"""
Function: My2DMatrix Compare with numpy
@author: LokiXun
@contact: 2682414501@qq.com
"""
from typing import List
import ctypes
from pathlib import Path
import sys
import time

import numpy as np

base_path = Path(__file__).parent.resolve()
dll_filepath = base_path.joinpath("MyMatrix.dll")
assert dll_filepath.exists()
print(dll_filepath.as_posix())

# check sys bit
system_bit = 64 if sys.maxsize > 0x100000000 else 32
sys_version_str = ''.join(item.strip() for item in sys.version.split('\\n'))
print(f"Python {sys_version_str} {1:03d}bit on {sys.platform:s}\n")

myMatrix_lib = ctypes.cdll.LoadLibrary(dll_filepath.as_posix())
print(f"load DLL success! {dll_filepath.as_posix()}")


class My2dMatrix:
    """current only support Int"""

    def __init__(self, matrix_2d_array: np.ndarray):
        """2D matrix realized in cpp"""
        assert len(matrix_2d_array.shape) == 2, "supposed to be 2d matrix"

        # we need to have a numpy contiguous array to pass into the C++ function
        self.matrix_np_array = np.ascontiguousarray(matrix_2d_array, np.int32)
        self.intPtr = ctypes.POINTER(ctypes.c_int)
        self.intPtrPtr = ctypes.POINTER(self.intPtr)
        self.ct_arr = np.ctypeslib.as_ctypes(self.matrix_np_array)
        self.intPtrArr = self.intPtr * self.ct_arr._length_
        self.ct_ptr = ctypes.cast(self.intPtrArr(*(ctypes.cast(row, self.intPtr) for row in self.ct_arr)),
                                  self.intPtrPtr)

        # define C++ class initialiser input/outputs types
        myMatrix_lib.init_myMatrix2D_int.argtypes = [self.intPtrPtr, ctypes.c_int, ctypes.c_int]
        myMatrix_lib.init_myMatrix2D_int.restype = ctypes.c_void_p
        # multiply
        myMatrix_lib.multiply_int.argtypes = [ctypes.c_void_p, ctypes.c_int]
        myMatrix_lib.multiply_int.restype = ctypes.c_void_p
        myMatrix_lib.multiply_2matrix.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        myMatrix_lib.multiply_2matrix.restype = ctypes.c_void_p

        self.matrix_obj = myMatrix_lib.init_myMatrix2D_int(self.ct_ptr,
                                                           self.matrix_np_array.shape[0],
                                                           self.matrix_np_array.shape[1])
        # print(f"type={type(self.matrix_obj)}, matrix_obj={self.matrix_obj}")

    @staticmethod
    def multiply_with_int(matrix_obj, int_number: int):
        """矩阵乘"""
        return myMatrix_lib.multiply_int(matrix_obj, int_number)

    @staticmethod
    def multiply_with_matrix(first_matrix_obj, second_matrix_obj):
        """矩阵*矩阵"""
        return myMatrix_lib.multiply_2matrix(first_matrix_obj, second_matrix_obj)


if __name__ == '__main__':
    # matrix_a = np.array([[1, 2, 3], [4, 5, 6]])
    # matrix_b = np.array([[1, 1], [2, 2], [3, 3]])
    np.random.seed(1)
    matrix_a = np.random.randint(1, 100, size=(100, 100))
    matrix_b = np.random.randint(1, 100, size=(100, 100))

    my_2d_matrix_a = My2dMatrix(matrix_a)
    my_2d_matrix_b = My2dMatrix(matrix_b)
    my_2d_matrix_result = My2dMatrix(np.array([[]]))

    multiply_int_data = 999
    my_2d_matrix_result.matrix_obj = My2dMatrix.multiply_with_int(
        my_2d_matrix_a.matrix_obj, multiply_int_data)  # 0.0181378s
    # My2dMatrix.multiply_with_int(my_2d_matrix_result.matrix_obj, 4)
    start_time = time.time_ns()
    result_matrix = matrix_a * multiply_int_data  # 0.0000000000s
    print(f"numpy multiply_with_int costs={(time.time_ns() - start_time) / (10 ** 9):.10f}s")

    my_2d_matrix_a.multiply_with_matrix(my_2d_matrix_a.matrix_obj, my_2d_matrix_b.matrix_obj)  # 0.0039863s
    start_time = time.time_ns()
    result_matrix = matrix_a @ matrix_b
    print(f"numpy multiply_with_matrix costs={(time.time_ns() - start_time) / (10 ** 9):.10f}s")  # 0.0009999000s
    print(f"result_matrix\n={result_matrix}")
