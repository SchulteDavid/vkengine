#ifndef MATRIX_H
#define MATRIX_H

#include <glm/glm.hpp>

#include <vector>
#include "vector.h"

#include "util/debug/trace_exception.h"

namespace Math {

template <unsigned int rows, unsigned int cols, typename T = double> class Matrix {

    public:
        Matrix() {

            for (int i = 0; i < rows * cols; ++i) {
                this->data[i] = (T) (i % (rows + cols + 1)? 1 : 0);
            }

        }

        Matrix(const T sFactor) {

            for (int i = 0; i < rows * cols; ++i) {
                this->data[i] = (T) (i % (rows + cols + 1)? sFactor : 0);
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
                throw dbg::trace_exception("Matrix index out of bounds");

            return Vector<cols, T>(this->data + i * cols);
        }

        T& operator() (int i, int j) {
            if (i < 0 || i >= rows || j < 0 || j >= rows)
                throw dbg::trace_exception("Matrix index out of bounds");
            return *(this->data + i * cols + j);
        }

        Vector<rows, T> operator()(int i) {
            if (i < 0 || i >= cols)
                throw dbg::trace_exception("Matrix index out of bounds");
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
                throw dbg::trace_exception("Waiting on Gaus-solving");
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
                throw dbg::trace_exception("LU only available for square matrices");

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

            if (rows != 4 && cols != 4)
                throw dbg::trace_exception("Unable to transform non 4x4 Matrix to glm::mat4");

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


#ifdef __SSE2__

#include <x86intrin.h>
#include <immintrin.h>

const float _matrix_init_data[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 0
};

template <> class Matrix<4,4,float> {

    public:

        Matrix() : Matrix(_matrix_init_data) {

        }

        Matrix(const float sFactor) {

            alignas(16) float data[16];

            for (int i = 0; i < 16; ++i) {
                data[i] = (i % 5 ? sFactor : 0);
            }

            for (unsigned int i = 0; i < 4; ++i) {
                this->rows[i] = _mm_load_ps(data + (4 * i));
            }

        }

        Matrix(const float * data) {

            for (unsigned int i = 0; i < 4; ++i) {
                this->rows[i] = _mm_load_ps(data + (4 * i));
            }

        }

        Matrix(__m128 r0, __m128 r1, __m128 r2, __m128 r3) {

            rows[0] = r0;
            rows[1] = r1;
            rows[2] = r2;
            rows[3] = r3;

        }

        static inline __m128 linComb(const __m128 & v_data, const Matrix<4,4,float> & m) {

            __m128 result;

            result = _mm_mul_ps(_mm_shuffle_ps(v_data, v_data, 0x00), m.rows[0]);
            result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(v_data, v_data, 0x55), m.rows[1]));
            result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(v_data, v_data, 0xaa), m.rows[2]));
            result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(v_data, v_data, 0xff), m.rows[3]));

            return result;

        }

        Matrix<4, 4, float> operator*(const Matrix<4, 4, float> & m) {

            __m128 r0 = linComb(rows[0], m);
            __m128 r1 = linComb(rows[1], m);
            __m128 r2 = linComb(rows[2], m);
            __m128 r3 = linComb(rows[3], m);

            return Matrix<4,4,float>(r0, r1, r2, r3);

        }

        Vector<4, float> operator*(Vector<4, float> v) {

            __m128 v_data = v.getVData();
            __m128 result = linComb(v_data, *this);

            float rData[4];
            _mm_store_ps(rData, result);

            return Vector<4, float>(rData);

        }

        const float operator() (int i, int j) {
            if (i < 0 || i >= 4 || j < 0 || j >= 4)
                throw dbg::trace_exception("Matrix index out of bounds");

            alignas(16) float data[4];
            _mm_store_ps(data, rows[i]);

            return *(data + j);
        }

        Vector<4, float> operator()(int i) {
            if (i < 0 || i >= 4)
                throw dbg::trace_exception("Matrix index out of bounds");

            float tmp[4];
            for (unsigned int j = 0; j < 4; ++j) {

                alignas(16) float data[4];
                _mm_store_ps(data, rows[j]);

                tmp[j] = data[i];

            }

            return Vector<4, float>(tmp);

        }

        Vector<4, float> operator[] (int i) {
            return Vector<4, float>(rows[i]);
        }

        glm::mat4 toGlmMatrix() {

            glm::mat4 mat = glm::mat4(1.0);
            mat[0] = glm::vec4((*this)(0,0), (*this)(1,0), (*this)(2,0), (*this)(3,0));
            mat[1] = glm::vec4((*this)(0,1), (*this)(1,1), (*this)(2,1), (*this)(3,1));
            mat[2] = glm::vec4((*this)(0,2), (*this)(1,2), (*this)(2,2), (*this)(3,2));
            mat[3] = glm::vec4((*this)(0,3), (*this)(1,3), (*this)(2,3), (*this)(3,3));

            return mat;

        }

        float det() {
            //return mDet(rows, cols, data);
            Matrix<4, 4, float> lu = LU();
            float res = (float) 1;
            for (int i = 0; i < 4; ++i) {
                res *= lu[i][i];
            }
            return res;
        }

        Vector<4, float> solve(Vector<4, float> vec) {

            Matrix<4, 4, float> lu = LU();

            return luSolve(lu, vec);

        }

        std::vector<Vector<4, float>> solve(std::vector<Vector<4, float>> vecs) {

            Matrix<4, 4, float> lu = LU();
            std::vector<Vector<4, float>> res(vecs.size());

            for (int i = 0; i < vecs.size(); ++i) {
                res[i] = luSolve(lu, vecs[i]);
            }

            return res;

        }

    private:

        void updateSSEData() {

            /*for (unsigned int i = 0; i < 4; ++i) {
                this->rows[i] = _mm_load_ps(this->data + (4 * i));
            }*/

        }

        Vector<4, float> luSolve(Matrix<4, 4, float> lu, Vector<4, float> vec) {

            float y[4];
            y[0] = vec[0];
            for (int i = 1; i < 4; ++i) {
                float t = 0;
                for (int j = 0; j < i; ++j) {
                    t += (i == j ? 1 : lu[i][j]) * y[j];
                }
                y[i] = vec[i] - t;
            }
            float x[4];
            x[4-1] = y[4-1] / lu[4-1][4-1];
            for (int i = 4-1; i >= 0; --i) {
                float t = 0;
                for (int j = i + 1; j < 4; ++j)
                    t += lu[i][j] * x[j];
                x[i] = (1 / lu[i][i]) * (y[i] - t);

            }

            return Vector<4,float>(x);

        }

        Matrix<4, 4, float> LU() {

            float res[16];
            for (int j = 0; j < 4; ++j) {

                for (int i = 0; i < 4; ++i) {

                    if (i <= j) {

                        res[ i * 4 + j] = (*this)[i][j] - getLUFactor(i, i, j, res);

                    } else {

                        res[ i * 4 + j] = ((*this)[i][j] - getLUFactor(j, i, j, res)) / res[j * 4 + j];

                    }

                }

            }

            return Matrix<4,4,float>(res);

        }

        float getLUFactor(int m, int i, int j, Matrix<4, 4, float> mat) {

            float res = 0;
            for (int k = 0; k < m; ++k) {

                res += (k == i ? 1 : mat[i][k]) * mat[k][j];

            }

            return res;

        }


        //alignas(16) float data[16];
        __m128 rows[4];

};

#endif // __SSE2__

template<unsigned int i, unsigned int j, typename T> Matrix<i,j,T> scaleMatrix(float f) {
    return Matrix<i, j, T>(f);
}

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
