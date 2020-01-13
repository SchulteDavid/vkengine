#ifndef QUATERNION_H
#define QUATERNION_H

#include "matrix.h"
#include "vector.h"

#include <glm/vec3.hpp>

namespace Math {

template <typename T = double> class Quaternion {

    public:

        T a;
        T b;
        T c;
        T d;

        /**
        Builds a quaternion from linear values
        **/
        Quaternion(T a, T b, T c, T d) {

            this->a = a;
            this->b = b;
            this->c = c;
            this->d = d;

        }

        Quaternion(T a, Vector<3, T> vec) {

            this->a = a;
            this->b = vec[0];
            this->c = vec[1];
            this->d = vec[2];

        }

        /**
        Builds a quaternion from axis-angle representation of rotation.
        **/
        Quaternion(Vector<3, T> axis, T angle) {

            T s = sin(angle / 2);
            b = axis[0] * s;
            c = axis[1] * s;
            d = axis[2] * s;

            a = cos(angle / 2);

        }

        /**
        Builds quaternion where all values are 0.
        **/
        Quaternion() {
            this->a = 0;
            this->b = 0;
            this->c = 0;
            this->d = 0;
        }

        virtual ~Quaternion() {

        }

        Vector<3, T> getVectorPart() {
            T tmp[3] = {b, c, d};
            return Vector<3, T>(tmp);
        }

        T getLinearPart() {
            return a;
        }

        Matrix<3, 3, T> toRotationMatrix() {

            std::vector<T> values(9);

            values[0] = a * a + b * b - c * c - d * d;
            values[1] = 2 * b * c - 2 * a * d;
            values[2] = 2 * b * d + 2 * a * c;
            values[3] = 2 * b * c + 2 * a * d;
            values[4] = a * a - b * b + c * c - d * d;
            values[5] = 2 * c * d - 2 * a * b;
            values[6] = 2 * b * d - 2 * a * c;
            values[7] = 2 * c * d + 2 * a * b;
            values[8] = a * a - b * b - c * c + d * d;

            return Matrix<3, 3, T>(values.data());

        }

        Matrix<4, 4, T> toModelMatrix(Vector<3, T> loc) {

            T s = 1 / (norm()*norm());

            std::vector<T> values(16);

            values[0] = 1 - 2 * s * (c*c + d*d);
            values[1] = 2 * s * (b*c - d*a);
            values[2] = 2 * s * (b*d + c*a);

            values[4] = 2 * s * (b*c + d*a);
            values[5] = 1 - 2 * s * (b*b + d*d);
            values[6] = 2 * s * (c*d - b * a);

            values[8] = 2 * s * (b*d - c * a);
            values[9] = 2 * s * (c*d + b * a);
            values[10] = 1 - 2 * s * (b*b + c*c);

            values[3] = loc.x;
            values[7] = loc.y;
            values[11] = loc.z;

            values[12] = 0;
            values[13] = 0;
            values[14] = 0;
            values[15] = 1;

            return Matrix<4, 4, T>(values.data());

        }

        Matrix<4, 4, T> toModelMatrix(glm::vec3 loc) {

            T s = 1 / (norm()*norm());

            std::vector<T> values(16);

            values[0] = 1 - 2 * s * (c*c + d*d);
            values[1] = 2 * s * (b*c - d*a);
            values[2] = 2 * s * (b*d + c*a);

            values[4] = 2 * s * (b*c + d*a);
            values[5] = 1 - 2 * s * (b*b + d*d);
            values[6] = 2 * s * (c*d - b * a);

            values[8] = 2 * s * (b*d - c * a);
            values[9] = 2 * s * (c*d + b * a);
            values[10] = 1 - 2 * s * (b*b + c*c);

            values[3] = loc.x;
            values[7] = loc.y;
            values[11] = loc.z;

            values[12] = 0;
            values[13] = 0;
            values[14] = 0;
            values[15] = 1;

            return Matrix<4, 4, T>(values.data());

        }

        Quaternion<T> operator+(Quaternion<T> q) {
            return Quaternion<T>(a + q.a, b + q.b, c + q.c, d + q.d);
        }

        Quaternion<T> operator+(T f) {
            return Quaternion<T>(a + f, b, c, d);
        }

        Quaternion<T> operator-(Quaternion<T> q) {

            return Quaternion<T>(a - q.a, b - q.b, c - q.c, d - q.d);

        }

        Quaternion<T> operator-(T f) {

            return Quaternion<T>(a - f, b, c, d);

        }

        Quaternion<T> operator=(T d) {
            return Quaternion<T>(d,0,0,0);
        }

        Quaternion<T> operator*(Quaternion<T> q) {

            Vector<3, T> v0 = this->getVectorPart();
            Vector<3, T> v1 = q.getVectorPart();

            return Quaternion<T>(a * q.a - v0 * v1, a * v1 + q.a * v0 + cross(v0, v1));

        }

        Quaternion<T> operator*(T d)  {
            return *this * Quaternion(d, 0, 0, 0);
        }

        Quaternion<T> operator/(Quaternion<T> q) {

            return *this * q.conjugate() / q.norm();

        }

        Quaternion<T> operator/(T d) {
            return Quaternion<T>(a/d, b/d, c/d, this->d / d);
        }

        Quaternion<T> conjugate() {

            return Quaternion<T>(a, -b, -c, -d);

        }

        T norm() {

            return (T) sqrt(a*a + b*b + c*c + d*d);

        }

        static Quaternion<T> fromAxisAngle(Vector<3, T> axis, double angle) {

            axis.normalize();

            return Quaternion(cos(angle / 2.0), axis * sin(angle / 2.0));

        }

};

}

#endif // QUATERNION_H
