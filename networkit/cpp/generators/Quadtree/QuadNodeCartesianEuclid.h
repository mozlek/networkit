/*
 * QuadNodePolarEuclid.h
 *
 *  Created on: 21.05.2014
 *      Author: Moritz v. Looz (moritz.looz-corswarem@kit.edu)
 *
 *  Note: This is similar enough to QuadNode.h that one could merge these two classes.
 */

#ifndef QUADNODECARTESIANEUCLID_H_
#define QUADNODECARTESIANEUCLID_H_

#include <vector>
#include <algorithm>
#include <functional>
#include <assert.h>
#include "../../auxiliary/Log.h"
#include "../../geometric/HyperbolicSpace.h"

using std::vector;
using std::min;
using std::max;
using std::cos;

namespace NetworKit {

template <class T>
class QuadNodeCartesianEuclid {
	friend class QuadTreeGTest;
private:
	double minX;
	double minY;
	double maxX;
	double maxY;
	unsigned capacity;
	static const unsigned coarsenLimit = 4;
	static const long unsigned sanityNodeLimit = 10E15; //just assuming, for debug purposes, that this algorithm never runs on machines with more than 4 Petabyte RAM
	count subTreeSize;
	std::vector<T> content;
	std::vector<Point2D<double> > positions;
	bool isLeaf;
	bool splitTheoretical;
	index ID;
	double lowerBoundR;

public:
	std::vector<QuadNodeCartesianEuclid> children;

	QuadNodeCartesianEuclid() {
		//This should never be called.
		minX = 0;
		maxX = 0;
		minY = 0;
		maxY = 0;
		capacity = 20;
		isLeaf = true;
		subTreeSize = 0;
		splitTheoretical = false;
		ID = 0;
	}

	/**
	 * Construct a QuadNode for polar coordinates.
	 *
	 *
	 * @param leftAngle Minimal angular coordinate of region, in radians from 0 to 2\pi
	 * @param rightAngle Maximal angular coordinate of region, in radians from 0 to 2\pi
	 * @param minR Minimal radial coordinate of region, between 0 and 1
	 * @param maxR Maximal radial coordinate of region, between 0 and 1
	 * @param capacity Number of points a leaf cell can store before splitting
	 * @param minDiameter Minimal diameter of a quadtree node. If the node is already smaller, don't split even if over capacity. Default is 0
	 * @param splitTheoretical Whether to split in a theoretically optimal way or in a way to decrease measured running times
	 * @param alpha dispersion Parameter of the point distribution. Only has an effect if theoretical split is true
	 * @param diagnostics Count how many necessary and unnecessary comparisons happen in leaf cells? Will cause race condition and false sharing in parallel use
	 *
	 */
	QuadNodeCartesianEuclid(Point2D<double> lower, Point2D<double> upper, unsigned capacity, bool splitTheoretical = false) {
		this->minX = lower.getX();
		this->minY = lower.getY();
		this->maxX = upper.getX();
		this->maxY = upper.getY();
		this->capacity = capacity;
		this->splitTheoretical = splitTheoretical;
		this->ID = 0;
		isLeaf = true;
		subTreeSize = 0;
	}

	void split() {
		assert(isLeaf);
		//heavy lifting: split up!
		double middleX, middleY;
		if (splitTheoretical) {
			//Euclidean space is distributed equally
			middleX = (minX + maxX) / 2;
			middleY = (minY + maxY) / 2;
		} else {
			//median of points
			const count numPoints = positions.size();
			vector<double> sortedX(numPoints);
			vector<double> sortedY(numPoints);
			for (index i = 0; i < numPoints; i++) {
				sortedX[i] = positions[i].getX();
				sortedY[i] = positions[i].getY();
			}
			std::sort(sortedX.begin(), sortedX.end());
			std::sort(sortedY.begin(), sortedY.end());
			assert(sortedX.size() == numPoints);
			assert(sortedY.size() == numPoints);
			middleX = sortedX[numPoints/2];
			middleY = sortedY[numPoints/2];
		}
		assert(middleX > minX);
		assert(middleX < maxX);
		assert(middleY > minY);
		assert(middleY < maxY);
		Point2D<double> middle(middleX, middleY);

		QuadNodeCartesianEuclid southwest(Point2D<double>(minX, minY), middle, capacity, splitTheoretical);
		QuadNodeCartesianEuclid southeast(Point2D<double>(middleX, minY), Point2D<double>(maxX, middleY), capacity, splitTheoretical);
		QuadNodeCartesianEuclid northwest(Point2D<double>(minX, middleY), Point2D<double>(middleX, maxY), capacity, splitTheoretical);
		QuadNodeCartesianEuclid northeast(middle, Point2D<double>(maxX, maxY), capacity, splitTheoretical);
		children = {southwest, southeast, northwest, northeast};
		for (auto child : children) assert(child.isLeaf);
		isLeaf = false;
	}

	/**
	 * Add a point at polar coordinates (angle, R) with content input. May split node if capacity is full
	 *
	 * @param input arbitrary content, in our case an index
	 * @param angle angular coordinate of point, between 0 and 2 pi.
	 * @param R radial coordinate of point, between 0 and 1.
	 */
	void addContent(T input, Point2D<double> pos) {
		assert(input < sanityNodeLimit);
		assert(content.size() == positions.size());
		assert(this->responsible(pos));
		if (isLeaf) {
			if (content.size() + 1 < capacity) {
				content.push_back(input);
				positions.push_back(pos);
			} else {
				split();

				for (index i = 0; i < content.size(); i++) {
					this->addContent(content[i], positions[i]);
				}
				assert(subTreeSize == content.size());//we have added everything twice
				subTreeSize = content.size();
				content.clear();
				positions.clear();
				this->addContent(input, pos);
			}
		}
		else {
			assert(children.size() > 0);
			bool foundResponsibleChild = false;
			for (index i = 0; i < children.size(); i++) {
				if (children[i].responsible(pos)) {
					foundResponsibleChild = true;
					children[i].addContent(input, pos);
					break;
				} else {
					TRACE("Child not responsible for (", pos.getX(), ", ", pos.getY(), ").");
				}
			}
			assert(foundResponsibleChild);
			subTreeSize++;
		}
	}

	/**
	 * Remove content at polar coordinates (angle, R). May cause coarsening of the quadtree
	 *
	 * @param input Content to be removed
	 * @param angle Angular coordinate
	 * @param R Radial coordinate
	 *
	 * @return True if content was found and removed, false otherwise
	 */
	bool removeContent(T input, Point2D<double> pos) {
		if (!responsible(pos)) return false;
		if (isLeaf) {
			index i = 0;
			for (; i < content.size(); i++) {
				if (content[i] == input) break;
			}
			if (i < content.size()) {
				assert(positions[i].distance(pos) == 0);
				//remove element
				content.erase(content.begin()+i);
				positions.erase(positions.begin()+i);
				return true;
			} else {
				return false;
			}
		}
		else {
			bool removed = false;
			bool allLeaves = true;
			assert(children.size() > 0);
			for (index i = 0; i < children.size(); i++) {
				if (!children[i].isLeaf) allLeaves = false;
				if (children[i].removeContent(input, pos)) {
					assert(!removed);
					removed = true;
				}
			}
			if (removed) subTreeSize--;
			//coarsen?
			if (removed && allLeaves && size() < coarsenLimit) {
				//coarsen!!
				//why not assert empty containers and then insert directly?
				vector<T> allContent;
				vector<Point2D<double> > allPositions;
				for (index i = 0; i < children.size(); i++) {
					allContent.insert(allContent.end(), children[i].content.begin(), children[i].content.end());
					allPositions.insert(allPositions.end(), children[i].positions.begin(), children[i].positions.end());
				}
				assert(allContent.size() == allPositions.size());
				children.clear();
				content.swap(allContent);
				positions.swap(allPositions);
				isLeaf = true;
			}

			return removed;
		}
	}


	/**
	 * Check whether the region managed by this node lies outside of an Euclidean circle.
	 *
	 * @param query Center of the Euclidean query circle, given in Cartesian coordinates
	 * @param radius Radius of the Euclidean query circle
	 *
	 * @return True if the region managed by this node lies completely outside of the circle
	 */
	bool outOfReach(Point2D<double> query, double radius) const {
		return EuclideanDistances(query).first > radius;
	}

	/**
	 * @param query Position of the query point
	 */
	std::pair<double, double> EuclideanDistances(Point2D<double> query) const {
		/**
		 * If the query point is not within the quadnode, the distance minimum is on the border.
		 * Need to check whether extremum is between corners.
		 */
		double maxDistance = 0;
		double minDistance = std::numeric_limits<double>::max();

		if (responsible(query)) minDistance = 0;

		auto updateMinMax = [&minDistance, &maxDistance, query](Point2D<double> pos){
			double extremalValue = pos.distance(query);
			maxDistance = std::max(extremalValue, maxDistance);
			minDistance = std::min(minDistance, extremalValue);
		};

		/**
		 * Horizontal boundaries
		 */
		if (query.getX() > minX && query.getX() < maxX) {
			Point2D<double> upper(query.getX(), maxY);
			Point2D<double> lower(query.getX(), minY);
			updateMinMax(upper);
			updateMinMax(lower);
		}

		/**
		 * Vertical boundaries
		 */
		if (query.getY() > minY && query.getY() < maxY) {
			Point2D<double> left(maxX, query.getY());
			Point2D<double> right(minX, query.getY());
			updateMinMax(left);
			updateMinMax(right);
		}


		/**
		 * corners
		 */
		updateMinMax(Point2D<double>(minX, minY));
		updateMinMax(Point2D<double>(minX, maxY));
		updateMinMax(Point2D<double>(maxX, minY));
		updateMinMax(Point2D<double>(maxX, maxY));

		assert(minDistance < query.length() + Point2D<double>(maxX, maxY).length());
		assert(minDistance < maxDistance);
		return std::pair<double, double>(minDistance, maxDistance);
	}


	/**
	 * Does the point at (angle, r) fall inside the region managed by this QuadNode?
	 *
	 * @param angle Angular coordinate of input point
	 * @param r Radial coordinate of input points
	 *
	 * @return True if input point lies within the region of this QuadNode
	 */
	bool responsible(Point2D<double> pos) const {
		return (pos.getX() >= minX && pos.getY() >= minY && pos.getX() < maxX && pos.getY() < maxY);
	}

	/**
	 * Get all Elements in this QuadNode or a descendant of it
	 *
	 * @return vector of content type T
	 */
	std::vector<T> getElements() const {
		if (isLeaf) {
			return content;
		} else {
			assert(content.size() == 0);
			assert(positions.size() == 0);
			vector<T> result;
			for (index i = 0; i < children.size(); i++) {
				std::vector<T> subresult = children[i].getElements();
				result.insert(result.end(), subresult.begin(), subresult.end());
			}
			return result;
		}
	}

	void getCoordinates(vector<Point2D<double> > &pointContainer) const {
		if (isLeaf) {
			pointContainer.insert(pointContainer.end(), positions.begin(), positions.end());
		}
		else {
			assert(content.size() == 0);
			assert(positions.size() == 0);
			for (index i = 0; i < children.size(); i++) {
				children[i].getCoordinates(pointContainer);
			}
		}
	}

	/**
	 * Main query method, get points lying in a Euclidean circle around the center point.
	 * Optional limits can be given to get a different result or to reduce unnecessary comparisons
	 *
	 * Elements are pushed onto a vector which is a required argument. This is done to reduce copying
	 *
	 * Safe to call in parallel.
	 *
	 * @param center Center of the query circle
	 * @param radius Radius of the query circle
	 * @param result Reference to the vector where the results will be stored
	 * @param minAngle Optional value for the minimum angular coordinate of the query region
	 * @param maxAngle Optional value for the maximum angular coordinate of the query region
	 * @param lowR Optional value for the minimum radial coordinate of the query region
	 * @param highR Optional value for the maximum radial coordinate of the query region
	 */
	void getElementsInEuclideanCircle(Point2D<double> center, double radius, vector<T> &result) const {
		if (outOfReach(center, radius)) {
			return;
		}

		if (isLeaf) {
			const double rsq = radius*radius;
			const double queryX = center[0];
			const double queryY = center[1];
			const count cSize = content.size();

			for (int i=0; i < cSize; i++) {
				const double deltaX = positions[i].getX() - queryX;
				const double deltaY = positions[i].getY() - queryY;
				if (deltaX*deltaX + deltaY*deltaY < rsq) {
					result.push_back(content[i]);
					if (content[i] >= sanityNodeLimit) DEBUG("Quadnode content ", content[i], " found, suspiciously high!");
					assert(content[i] < sanityNodeLimit);
				}
			}
		}	else {
			for (index i = 0; i < children.size(); i++) {
				children[i].getElementsInEuclideanCircle(center, radius, result);
			}
		}
	}

	count getElementsProbabilistically(Point2D<double> euQuery, std::function<double(double)> prob, vector<T> &result) const {
		TRACE("Getting Euclidean distances");
		auto distancePair = EuclideanDistances(euQuery);
		double probUB = prob(distancePair.first);
		double probLB = prob(distancePair.second);
		assert(probLB <= probUB);
		if (probUB > 0.5) probUB = 1;
		if (probUB == 0) return 0;
		//TODO: return whole if probLB == 1
		double probdenom = std::log(1-probUB);
		if (probdenom == 0) return 0;//there is a very small probability, but we cannot process it.
		TRACE("probUB: ", probUB, ", probdenom: ", probdenom);

		count expectedNeighbours = probUB*size();
		count candidatesTested = 0;
		count incomingNeighbours = result.size();
		count ownsize = size();


		if (isLeaf) {
			const count lsize = content.size();
			TRACE("Leaf of size ", lsize);
			for (int i = 0; i < lsize; i++) {
				//jump!
				if (probUB < 1) {
					double random = Aux::Random::real();
					double delta = std::log(random) / probdenom;
					assert(delta >= 0);
					i += delta;
					if (i >= lsize) break;
					TRACE("Jumped with delta ", delta, " arrived at ", i);
				}
				assert(i >= 0);

				//see where we've arrived
				candidatesTested++;
				double distance = positions[i].distance(euQuery);
				assert(distance >= distancePair.first);//TODO: These should not fail!
				assert(distance <= distancePair.second);
				double q = prob(distance);
				q = q / probUB; //since the candidate was selected by the jumping process, we have to adjust the probabilities
				assert(q <= 1);

				//accept?
				double acc = Aux::Random::real();
				if (acc < q) {
					TRACE("Accepted node ", i, " with probability ", q, ".");
					result.push_back(content[i]);
				}
			}
		}	else {
			if (expectedNeighbours < 4 || probUB < 1/1000) {//select candidates directly instead of calling recursively
				TRACE("probUB = ", probUB,  ", switching to direct candidate selection.");
				assert(probUB < 1);
				const count stsize = size();
				for (index i = 0; i < stsize; i++) {
					double delta = std::log(Aux::Random::real()) / probdenom;
					assert(delta >= 0);
					i += delta;
					TRACE("Jumped with delta ", delta, " arrived at ", i, ". Calling maybeGetKthElement.");
					if (i < size()) maybeGetKthElement(probUB, euQuery, prob, i, result);//this could be optimized. As of now, the offset is subtracted separately for each point
					else break;
					candidatesTested++;
				}
			} else {//carry on as normal
				for (index i = 0; i < children.size(); i++) {
					TRACE("Recursively calling child ", i);
					candidatesTested += children[i].getElementsProbabilistically(euQuery, prob, result);
				}
			}
		}
		count finalNeighbours = result.size();
		if (probLB == 1) assert(finalNeighbours == incomingNeighbours + ownsize);
		return candidatesTested;
	}


	void maybeGetKthElement(double upperBound, Point2D<double> euQuery, std::function<double(double)> prob, index k, vector<T> &circleDenizens) const {
		TRACE("Maybe get element ", k, " with upper Bound ", upperBound);
		assert(k < size());
		if (isLeaf) {
			double acceptance = prob(euQuery.distance(positions[k]))/upperBound;
			TRACE("Is leaf, accept with ", acceptance);
			if (Aux::Random::real() < acceptance) circleDenizens.push_back(content[k]);
		} else {
			TRACE("Call recursively.");
			index offset = 0;
			for (index i = 0; i < children.size(); i++) {
				count childsize = children[i].size();
				if (k - offset < childsize) {
					children[i].maybeGetKthElement(upperBound, euQuery, prob, k - offset, circleDenizens);
					break;
				}
				offset += childsize;
			}
		}
	}

	/**
	 * Shrink all vectors in this subtree to fit the content.
	 * Call after quadtree construction is complete, causes better memory usage and cache efficiency
	 */
	void trim() {
		content.shrink_to_fit();
		positions.shrink_to_fit();
		if (!isLeaf) {
			for (index i = 0; i < children.size(); i++) {
				children[i].trim();
			}
		}
	}

	/**
	 * Number of points lying in the region managed by this QuadNode
	 */
	count size() const {
		return isLeaf ? content.size() : subTreeSize;
	}

	void recount() {
		subTreeSize = 0;
		for (index i = 0; i < children.size(); i++) {
			children[i].recount();
			subTreeSize += children[i].size();
		}
	}

	/**
	 * Height of subtree hanging from this QuadNode
	 */
	count height() const {
		count result = 1;//if leaf node, the children loop will not execute
		for (auto child : children) result = std::max(result, child.height()+1);
		return result;
	}

	/**
	 * Leaf cells in the subtree hanging from this QuadNode
	 */
	count countLeaves() const {
		if (isLeaf) return 1;
		count result = 0;
		for (index i = 0; i < children.size(); i++) {
			result += children[i].countLeaves();
		}
		return result;
	}

	index getID() const {
		return ID;
	}

	index indexSubtree(index nextID) {
		index result = nextID;
		assert(children.size() == 4 || children.size() == 0);
		for (int i = 0; i < children.size(); i++) {
			result = children[i].indexSubtree(result);
		}
		this->ID = result;
		return result+1;
	}

	index getCellID(Point2D<double> pos) const {
		if (!responsible(pos)) return -1;
		if (isLeaf) return getID();
		else {
			for (int i = 0; i < 4; i++) {
				index childresult = children[i].getCellID(pos);
				if (childresult >= 0) return childresult;
			}
			assert(false); //if responsible
			return -1;
		}
	}

	index getMaxIDInSubtree() const {
		if (isLeaf) return getID();
		else {
			index result = -1;
			for (int i = 0; i < 4; i++) {
				result = std::max(children[i].getMaxIDInSubtree(), result);
			}
			return std::max(result, getID());
		}
	}

	count reindex(count offset) {
		if (isLeaf)
		{
			#pragma omp task
			{
				index p = offset;
				std::generate(content.begin(), content.end(), [&p](){return p++;});
			}
			offset += size();
		} else {
			for (int i = 0; i < 4; i++) {
				offset = children[i].reindex(offset);
			}
		}
		return offset;
	}

	void sortPointsInLeaves() {
		if (isLeaf) {
			#pragma omp task
			{
				count cs = content.size();
				vector<index> permutation(cs);

				index p = 0;
				std::generate(permutation.begin(), permutation.end(), [&p](){return p++;});

				//can probably be parallelized easily, but doesn't bring much benefit
				std::sort(permutation.begin(), permutation.end(), [this](index i, index j){return positions[i].getX() < positions[j].getX();});

				//There ought to be a way to do this more elegant with some algorithm header, but I didn't find any

				std::vector<T> contentcopy(cs);
				std::vector<Point2D<double> > positioncopy(cs);

				for (index i = 0; i < cs; i++) {
					const index perm = permutation[i];
					contentcopy[i] = content[perm];
					positioncopy[i] = positions[perm];
				}

				content.swap(contentcopy);
				positions.swap(positioncopy);
			}

		} else {
			for (int i = 0; i < 4; i++) {
				children[i].sortPointsInLeaves();
			}
		}
	}
};
}

#endif /* QUADNODE_H_ */