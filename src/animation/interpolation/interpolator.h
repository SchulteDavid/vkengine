#ifndef _INTERPOLATOR_H
#define _INTERPOLATOR_H

#include <vector>
#include <mathutils/vector.h>
#include <mathutils/vectorspline.h>

template <typename T> class Interpolator {

public:
  Interpolator(const std::vector<double> & xValues, const std::vector<T> & yValues) : xValues(xValues), yValues(yValues) {
    
  };
  
  virtual ~Interpolator() {};

  inline T operator() (double x) const {
    return interpolate(x);
  }
  
protected:
  virtual T interpolate(double x) const = 0;

  void findIndices(double x, int & klo, int & khi) const {
    klo = 0;
    khi = this->xValues.size()-1;

    while (khi - klo > 1) {
      int k = (khi+klo)/2;
      if (this->xValues[k] > x)
	khi = k;
      else {

	if (klo == k) khi--;
	klo = k;

      }
    }
  }

  const std::vector<double> & xValues;
  const std::vector<T> & yValues;
  
};

template <typename T> class LinearInterpolator : public Interpolator<T> {
  
public:
  LinearInterpolator(const std::vector<double> & xValues, const std::vector<T> & yValues) : Interpolator<T>(xValues, yValues) {

  }

protected:
  T interpolate(double x) const {

    int klo, khi;
    this->findIndices(x, klo, khi);

    double f = (x - this->xValues[klo]) / (this->xValues[khi] - this->xValues[klo]);
    
    return (f * this->yValues[khi]) + ((1 - f) * this->yValues[klo]);
    
  }
  
};

template<unsigned int dim, typename T = double> class SplineInterpolator : public Interpolator<Math::Vector<dim, T>> {

public:
  SplineInterpolator(const std::vector<double> & xValues, const std::vector<Math::Vector<dim, T>> & yValues) : Interpolator<Math::Vector<dim, T>>(xValues, yValues), spline(xValues, yValues) {
    //splines = std::vector<Math::Spline<T>>(dim);

    /*for (unsigned int i = 0; i < dim; ++i) {

      std::vector<T> y(yValues.size());
      for (unsigned int j = 0; j < yValues.size(); ++j)
	y[i] = yValues[j](i);
      
      //splines[i] = Math::Spline(xValues, y);
      splines.push_back(Math::Spline(xValues, y));
      }*/
    //spline = Math::Spline<Math::Vector<dim, T>>(xValues, yValues);
    
  }

protected:
  Math::Vector<dim, T> interpolate(double x) const {

    /*std::vector<T> vals(dim);

    for (unsigned int i = 0; i < dim; ++i) {
      const Math::Spline<T> & sp = splines[i];
      vals[i] = sp(x);
      }*/
    
    //return Math::Vector<dim, T>(vals);

    return spline(x);
  }

private:
  Math::VectorSpline<dim, T> spline;
  
};

#endif
