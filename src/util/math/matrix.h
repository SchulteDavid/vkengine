#ifndef MATRIX_H
#define MATRIX_H

#include <glm/glm.hpp>

#include <vector>
#include "vector.h"

namespace Math {

template <unsigned int rows, unsigned int cols, typename T = double> class Matrix {

    public:
        Matrix() {

            for (int i = 0; i < rows * cols; ++i) {
                this->data[i] = (T) (i % (rows + cols + 1)? 1 : 0);
            }

        }

        /** Data sorted by rows, It looks correct when you write it:

            (0, 1)
            (1, 0)

        is optained by passing {0, 1, 0, 1}

        */
        Matrix(T * data) {

            for (int i = 0; i < rows * cols; ++i) {
                this->data[i] = data[i];
            }

        }
        virtual ~Matrix() {
        }

        Vector<cols, T> operator[](int i) {
            if (i < 0 || i >= rows)
                throw std::runtime_error("Matrix index out of bounds");

            return Vector<cols, T>(this->data + i * cols);
        }

        T& operator() (int i, int j) {
            if (i < 0 || i >= rows || j < 0 || j >= rows)
                throw std::runtime_error("Matrix index out of bounds");
            return *(this->data + i * cols + j);
        }

        Vector<rows, T> operator()(int i) {
            if (i < 0 || i >= cols)
                throw std::runtime_error("Matrix index out of bounds");
            T tmp[rows];
            for (int j = 0; j < rows; ++j) {
                tmp[j] = data[j*cols + i];
            }
            return Vector<rows, T>(tmp);
        }

        Vector<rows, T> operator*(Vector<cols, T> vec) {
            T tmp[rows];
            for (int i = 0; i < rows; ++i) {
                tmp[i] = (*this)[i] * vec;
            }
            return Vector<rows, T>(tmp);

        }

        T det() {
            //return mDet(rows, cols, data);
            Matrix<rows, cols, T> lu = LU();
            T res = (T) 1;
            for (int i = 0; i < cols; ++i) {
                res *= lu[i][i];
            }
            return res;
        }

        Matrix<rows, cols, T> withColumn(int c, Vector<rows, T> vec) {

            T tmp[rows * cols];
            for (int i = 0; i < rows; ++i) {

                for (int j = 0; j < cols; ++j) {

                    if (j == c) {
                        tmp[i*cols+j] = vec[i];
                    } else {
                        tmp[i*cols+j] = data[i*cols+j];
                    }

                }

            }
            return Matrix<rows, cols, T>(tmp);

        }

        Vector<cols, T> solve(Vector<rows, T> vec) {

            if (rows == cols) {

                Matrix<rows, cols, T> lu = LU();

                return luSolve(lu, vec);

            } else {
                throw std::runtime_error("Waiting on Gaus-solving");
            }

        }

        std::vector<Vector<cols, T>> solve(std::vector<Vector<rows, T>> vecs) {

            if (rows == cols) {
                Matrix<rows, cols, T> lu = LU();
                std::vector<Vector<cols, T>> res;
                for (int i = 0; i < vecs.size(); ++i) {
                    res[i] = luSolve(lu, vecs[i]);
                }
                return res;
            }

        }

        /*Matrix<rows, cols, T> invert() {

            if (rows != cols)
                throw std::runtime_error("Inverting non-square matrix");



        }*/

        Matrix<rows, cols, T> LU() {

            if (rows != cols)
                throw std::runtime_error("LU only available for square matrices");

            Matrix<rows, cols, T> res;
            for (int j = 0; j < cols; ++j) {

                for (int i = 0; i < rows; ++i) {

                    if (i <= j) {

                        res(i,j) = (*this)[i][j] - getLUFactor(i, i, j, res);

                    } else {

                        res(i,j) = ((*this)[i][j] - getLUFactor(j, i, j, res)) / res[j][j];

                    }

                }

            }

            return res;

        }

        glm::mat4 toGlmMatrix() {
            if (rows != 4 || cols != 4)
                throw std::runtime_error("Matrix cannot be converted to 4x4");

            glm::mat4 mat = glm::mat4(1.0);
            mat[0] = glm::vec4((*this)(0,0), (*this)(1,0), (*this)(2,0), (*this)(3,0));
            mat[1] = glm::vec4((*this)(0,1), (*this)(1,1), (*this)(2,1), (*this)(3,1));
            mat[2] = glm::vec4((*this)(0,2), (*this)(1,2), (*this)(2,2), (*this)(3,2));
            mat[3] = glm::vec4((*this)(0,3), (*this)(1,3), (*this)(2,3), (*this)(3,3));

            return mat;

        }

    protected:

    private:

        Vector<cols, T> solveKramer(Vector<rows, T> vec) {
            T d = det();
            Vector<cols, T> v(nullptr);
            for (int i = 0; i < cols; ++i) {
                v[i] = withColumn(i, vec).det() / d;
            }
            return v;
        }

        T mDet(int r, int c, T * idata) {

            if (r == 2) {
                return idata[0] * idata[3] - idata[1] * idata[2];
            }

            T val = (T) 0;
            for (int i = 0; i < c; ++i) {
                T * tmp = mForDet(r, c, i, idata);
                val += (i % 2 ? -1 : 1) * idata[i] * mDet(r-1, c-1, tmp);
                delete tmp;
            }
            return val;

        }

        T * mForDet(int r, int c, int k, T * iData) {

            T * tmp = new T[(r-1) * (c-1)];
            int index = 0;
            for (int i = 1; i < r; ++i) {
                for (int j = 0; j < c; ++j) {
                    if (k != j)
                        tmp[index++] = iData[i*r+j];
                }

            }
            return tmp;

        }

        T getLUFactor(int m, int i, int j, Matrix<rows, cols, T> mat) {

            T res = (T) 0;
            for (int k = 0; k < m; ++k) {

                res += (k == i ? (T)1 : mat[i][k]) * mat[k][j];

            }

            return res;

        }

        Vector<cols, T> luSolve(Matrix<rows, cols, T> lu, Vector<rows, T> vec) {

            Vector<cols, T> y;
            y[0] = vec[0];
            for (int i = 1; i < cols; ++i) {
                T t = (T) 0;
                for (int j = 0; j < i; ++j) {
                    t += (i == j ? (T) 1 : lu[i][j]) * y[j];
                }
                y[i] = vec[i] - t;
            }
            Vector<cols, T> x;
            x[cols-1] = y[cols-1] / lu[cols-1][cols-1];
            for (int i = cols-1; i >= 0; --i) {
                T t = (T) 0;
                for (int j = i + 1; j < cols; ++j)
                    t += lu[i][j] * x[j];
                x[i] = ((T)1 / lu[i][i]) * (y[i] - t);

            }
            return x;

        }

        T data[rows * cols];

};

}

template <unsigned int rows, unsigned int cols, typename T> inline std::ostream& operator<<(std::ostream& stream, Math::Matrix<rows, cols, T> mat) {
    stream << "|";
    stream << mat[0];
    for (int i = 1; i < rows; ++i) {
        stream << "|\n|" << mat[i];
    }
    stream << "|";
    return stream;
}
#endif // MATRIX_H
