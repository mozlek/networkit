/*
 * RcmMapper.h
 *
 *  Created on: Jul 2, 2013
 *      Author: Henning
 */

#ifndef RCMMAPPER_H_
#define RCMMAPPER_H_

#include "StaticMapper.h"

namespace NetworKit {

/**
 * TODO: class documentation
 */
class RcmMapper: public NetworKit::StaticMapper {
public:
	RcmMapper();
	virtual ~RcmMapper();

	virtual Mapping run(Graph& guest, Graph& host);

	template <typename L> void forEdgesOfInDegreeIncreasingOrder(const Graph& graph, node u, L handle) const;

	Permutation permute(const Graph& graph) const;
	Permutation invert(const Permutation& piIn) const;
};

} /* namespace NetworKit */
#endif /* RCMMAPPER_H_ */