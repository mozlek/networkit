/*
 * BFS.h
 *
 *  Created on: Jul 23, 2013
 *      Author: Henning
 */

#ifndef BFS_H_
#define BFS_H_

#include "Graph.h"
#include "SSSP.h"

namespace NetworKit {

/**
 * @ingroup graph
 * The BFS class is used to do a breadth-first search on a Graph from a given source node.
 */
class BFS : public SSSP {
public:
	/**
	 * Constructs the BFS class for @a G and source node @a source.
	 *
	 * @param G The graph.
	 * @param source The source node of the breadth-first search.
	 */
	BFS(const Graph& G, node source); 

	/**
	 * Breadth-first search from @a source.
	 * @return Vector of unweighted distances from node @a source, i.e. the
	 * length (number of edges) of the shortest path from @a source to any other node.
	 */
	virtual void run();

	virtual void runUntil(node t);

	virtual void runBidirectional(node t);

	node settleNext();

	void init(node s);

	node isFinished();

	count getCurrentMin();

	bool wasVisited(node u);
	
	edgeweight extractDistance(node u);

private:
	std::queue<node> q;

};
} /* namespace NetworKit */
#endif /* BFS_H_ */
