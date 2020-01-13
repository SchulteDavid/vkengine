#ifndef VECTOR_H
#define VECTOR_H

#include <cstdarg>
#include <cmath>
#include <ostream>
#include <iostream>

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
                throw std::runtime_error("No such coordinate");
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

    protected:

    private:

        T data[dim];

};

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
