#ifndef VECTOR_H
#define VECTOR_H

#include <cstdarg>
#include <cmath>
#include <ostream>
#include <iostream>

#include "util/debug/trace_exception.h"

namespace Math {

template<unsigned int dim, typename T = double> class Vector {

    public:
        /**This will copy data, you can free it after the call*/
        Vector(T * data){
            for (int i = 0; i < dim; ++i) {
                this->data[i] = data ? data[i] : (T) 0;
            }
        }

        Vector() {
            for (int i = 0; i < dim; ++i)
                this->data[i] = (T) 0;
        }

        /*Vector(T t0, ...) {

            std::cout << "Creating vector from initialiser list" << std::endl;

            this->data[0] = t0;
            va_list vl;
            va_start(vl, t0);
            for (int i = 1; i < dim; ++i) {
                std::cout << "Loading argument " << i << std::endl;
                T tmp = va_arg(vl, T);
                this->data[i] = tmp;
            }
            va_end(vl);

            std::cout << "Done" << std::endl;

        }*/

        virtual ~Vector() {

        }

        T& operator[](int i) {
            if (i < 0 || i >= dim)
                throw dbg::trace_exception("No such coordinate");
            return data[i];
        }

        T operator*(Vector<dim, T> vec) {
            T val = (T) 0;
            for (int i = 0; i < dim; ++i) {
                val = val + this->data[i] * vec.data[i];
            }
            return val;
        }

        Vector<dim, T> operator+(Vector<dim, T> vec) {

            T tmp[dim];
            for (int i = 0; i < dim; ++i) {
                tmp[i] = data[i] + vec.data[i];
            }
            return Vector<dim, T>(tmp);

        }

        Vector<dim, T> operator*(T v) {

            T tmp[dim];
            for (int i = 0; i < dim; ++i) {
                tmp[i] = data[i] *v;
            }
            return Vector<dim, T>(tmp);

        }

        Vector<dim, T> operator-(Vector<dim, T> vec) {

            T tmp[dim];
            for (int i = 0; i < dim; ++i) {
                tmp[i] = data[i] - vec.data[i];
            }
            return Vector<dim, T>(tmp);

        }

        Vector<dim, T> operator/(T v) {

            T tmp[dim];
            for (int i = 0; i < dim; ++i) {
                tmp[i] = data[i] / v;
            }
            return Vector<dim, T>(tmp);

        }

        void normalize() {
            T val = this->length();
            for (int i = 0; i < dim; ++i) {
                this->data[i] /= val;
            }
        }

        T length() {
            T sum = (T) 0;
            for (int i = 0; i < dim; ++i) {
                sum += this->data[i] * this->data[i];
            }
            return (T) sqrt(sum);
        }

        const T * getData() {
            return data;
        }

    protected:

    private:

        T data[dim];

};

#ifdef __SSE2__

#include <x86intrin.h>
#include <immintrin.h>

template <> class Vector<4, float> {

    public:

        Vector() : Vector(0,0,0,0) {

        }

        Vector(float x, float y, float z, float w) {

            float data[4];

            data[0] = x;
            data[1] = y;
            data[2] = z;
            data[3] = w;

            vData = _mm_load_ps(data);

        }

        Vector(float * data) {

            alignas(16) float tmp[4];

            tmp[0] = data[0];
            tmp[1] = data[1];
            tmp[2] = data[2];
            tmp[3] = data[3];

            vData = _mm_load_ps(tmp);

        }

        Vector(__m128 vData) {
            this->vData = vData;
        }

        /*const float * getData() {
            return data;
        }*/

        __m128 getVData() {
            return vData;
        }

        const float operator[](int i) {
            if (i < 0 || i >= 4)
                throw dbg::trace_exception("No such coordinate");

            float data[4];
            _mm_store_ps(data, vData);
            return data[i];
        }

        float operator*(Vector<4, float> vec) {

            __m128 v0 = _mm_mul_ps(vData, vec.vData);
            v0 = _mm_add_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(2, 3, 0, 1)), vec.vData);
            v0 = _mm_add_ps(v0, _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(0, 1, 2, 3)));

            float tmp[4];

            _mm_store_ps(tmp, v0);

            return tmp[0];

        }

        Vector<4, float> operator+(Vector<4, float> vec) {

            return Vector<4, float>(_mm_add_ps(vData, vec.vData));

        }

        Vector<4, float> operator*(float v) {

            __m128 vv = _mm_load_ps1(&v);

            return Vector<4, float>(_mm_mul_ps(vData, vv));

        }

        Vector<4, float> operator-(Vector<4, float> vec) {

            return Vector<4, float>(_mm_sub_ps(vData, vec.vData));

        }

        Vector<4, float> operator/(float v) {

            __m128 vv = _mm_load_ps1(&v);
            return Vector<4, float>(_mm_div_ps(vData, vv));

        }

        void normalize() {
            float val = this->length();
            __m128 vv = _mm_load_ps1(&val);
            this->vData = _mm_div_ps(vData, vv);
        }

        float length() {

            __m128 tmp = _mm_mul_ps(vData, vData);

            float data[4];
            _mm_store_ps(data, tmp);

            return (float) sqrt(data[0] + data[1] + data[2] + data[3]);
        }

    private:
        //float data[4];
        __m128 vData;

};

#endif

template <typename T> Vector<3, T> cross(Vector<3, T> vec1, Vector<3, T> vec2) {

    T tmp[3] = {vec1[1] * vec2[2] - vec1[2] * vec2[1], -vec1[2] * vec2[0] + vec1[0] * vec2[2], vec1[0] * vec2[1] - vec1[1] * vec2[0]};

    return Vector<3, T>(tmp);

}

}

template <unsigned int dim, typename T> inline Math::Vector<dim, T> operator*(T a, Math::Vector<dim, T> vec) {
    T tmp[dim];
    for (int i = 0; i < dim; ++i) {
        tmp[i] = vec[i] * a;
    }
    return Math::Vector<dim, T>(tmp);
}

template <unsigned int dim, typename T> inline std::ostream& operator<<(std::ostream& stream, Math::Vector<dim, T> vec) {
    stream << "(";
    stream << vec[0];
    for (int i = 1; i < dim; ++i) {
        stream << ", " << vec[i];
    }
    stream << ")";
    return stream;
}

#endif // VECTOR_H
