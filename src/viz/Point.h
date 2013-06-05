/*
 * Point.h
 *
 *  Created on: Apr 11, 2013
 *      Author: Henning
 */

#ifndef POINT_H_
#define POINT_H_

#include <vector>
#include <cinttypes>
#include <cassert>
#include <cmath>

namespace NetworKit {

typedef int64_t index; // more expressive name for an index into an array
typedef int64_t count; // more expressive name for an integer quantity


/**
 * Points in any dimension of templated type.
 */
template<class T>
class Point {
protected:
	std::vector<T> data;

public:
	Point() { data = {0.0, 0.0}; }
	Point(std::vector<T>& values): data(values) {}
	virtual ~Point() {}

	count getDimensions() { return data.size(); }
	T getValue(index dim) { return data.at(dim); }
	void setValue(index dim, T value) { data.at(dim) = value; }

	T distance(const Point<T>& p) const;
	T squaredDistance(const Point<T>& p) const;

	Point& operator+=(const Point<T>& p);
	Point& scale(const T factor);

	T length() const;
};

template<class T>
T Point<T>::length() const {
	T length = (T) 0;
	for (index i = 0; i < data.size(); ++i) {
		T diff = this->data[i];
		length += diff * diff;
	}
	return sqrt(length);
}

template<class T>
T Point<T>::squaredDistance(const Point<T>& p) const {
	assert(this->data.size() == p.data.size());
	T dist = (T) 0;
	for (index i = 0; i < data.size(); ++i) {
		T diff = this->data[i] - p.data[i];
		dist += diff * diff;
	}
	return dist;
}

template<class T>
T Point<T>::distance(const Point<T>& p) const {
	return sqrt(squaredDistance(p));
}

template<class T>
Point<T>& Point<T>::operator+=(const Point<T>& p) {
	assert(this->data.size() == p.data.size());
	for (index i = 0; i < data.size(); ++i) {
		this->data[i] += p.data[i];
	}
	return *this;
}

template<class T>
Point<T>& Point<T>::scale(const T factor) {
	for (index i = 0; i < data.size(); ++i) {
		this->data[i] *= factor;
	}
	return *this;
}


} /* namespace NetworKit */
#endif /* POINT_H_ */
