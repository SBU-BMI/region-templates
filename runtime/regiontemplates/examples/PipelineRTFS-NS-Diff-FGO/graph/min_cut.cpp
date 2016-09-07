#include <iostream>
#include <map>

#include "min_cut.hpp"

using namespace mincut;

/**************************************************/
/**            Internal Funs Prototypes          **/
/**************************************************/

cut_t min_cut_phase(size_t n, weight_t** adjMat, 
	std::map<_id_t,std::list<_id_t>> &vertices, _id_t a);

weight_t sum(size_t n, weight_t* A);

/**************************************************/
/**                   Main code                  **/
/**************************************************/

// this is a test code for min_cut
int main() {
	size_t n=8;
	// weight_t _adjMat[n][n]={{0,2,2,1,1},
	// 						{2,0,2,1,1},
	// 						{2,2,0,1,1},
	// 						{1,1,1,0,3},
	// 						{1,1,1,3,0}};
	weight_t _adjMat[n][n]={{0,2,0,0,3,0,0,0},
							{2,0,3,0,2,2,0,0},
							{0,3,0,4,0,0,2,0},
							{0,0,4,0,0,0,2,2},
							{3,2,0,0,0,3,0,0},
							{0,2,0,0,3,0,1,0},
							{0,0,2,2,0,1,0,3},
							{0,0,0,2,0,0,3,0}};
	
	// my compiler won't do VLA, so we need this workaround
	weight_t** adjMat = new weight_t *[n];
	for (size_t i=0; i<n; i++) {
		adjMat[i] = new weight_t [n];
		for (size_t j=0; j<n; j++)
			adjMat[i][j] = _adjMat[i][j];
	}

	std::cout << "Cuts:" << std::endl;
	for (cut_t c : min_cut(n, adjMat)) {
		std::cout << "Got cut " << _cut_w(c) << ":" << std::endl;
		std::cout << "\tS1:" << std::endl;
		for (_id_t id : _cut_s1(c))
			std::cout << "\t\t" << id << std::endl;
		std::cout << "\tS2:" << std::endl;
		for (_id_t id : _cut_s2(c))
			std::cout << "\t\t" << id << std::endl;
	}

}

std::list<cut_t> mincut::min_cut(size_t n, weight_t** adjMat) {
	
	_id_t a;
	cut_t cut;
	std::list<cut_t> cuts;

	// initialize vertices
	std::map<_id_t, std::list<_id_t>> vertices;
	for (int i=0; i<n; i++)
		vertices[i] = std::list<_id_t>(1,i);

	// keep finding cuts until there are no more find
	while (vertices.size() > 1) {
		// get the first 'a' vertex as the most connected vertex
		weight_t max = 0;
		weight_t m;
		for (int i=0; i<n; i++) {
			if ((m = sum(n, adjMat[i])) > max) {
				a = i;
				max = m;
			}
		}

		// run a phase of cut
		cuts.emplace_back(min_cut_phase(n, adjMat, vertices, a));
	}

	return cuts;
}

cut_t min_cut_phase(size_t n_max, weight_t** adjMat, 
		std::map<_id_t,std::list<_id_t>> &vertices, _id_t a) {
	
	_id_t s;
	_id_t t;
	weight_t cut;
	size_t n = vertices.size();

	// generate priority queue
	fibonacci q;
	for (std::pair<_id_t, std::list<_id_t>> v : vertices) {
		if (v.first != a) {
			q.push(_node(adjMat[v.first][a], v.first));
		}
	}

	// array of vertices ids A
	_id_t A[n];
	A[0] = a;
	t = a;

	// std::cout << "a=" << a << std::endl; 
	// greedy insertion loop
	for (size_t i=1; i<n; i++) {
		// add the most tightly connected vertex to A
		// std::cout << "popping " << q.top().second << " of cost " 
		// 	<< q.top().first << std::endl;
		A[i] = q.top().second;
		q.pop();

		// The update queue must be done in two steps:
		//  1 - calculate updated values for the heap
		//  2 - actually updating the heap
		// This must be done because when calling 'increase' the 
		// iterator for the heap is invalidated, causing
		// undefined behaviour.

		// 1 - calculate updated values for the heap
		fibonacci::handle_type fibonacci_handlers[n-i-1];
		node_t fibonacci_updated[n-i]; 
		int j=0;
		for (fibonacci::iterator it=q.begin(); it!=q.end(); it++) {
			fibonacci_handlers[j] = q.s_handle_from_iterator(it);
			fibonacci_updated[j] = _node(it->first+adjMat[A[i]][it->second], it->second);
			// std::cout << "updated: " << fibonacci_updated[j].second << ":" 
			// 	<< fibonacci_updated[j].first << std::endl;
			j++;
		}

		// 2 - actually updates the heap
		for (j=0; j<n-i-1; j++) {
			q.increase(fibonacci_handlers[j], fibonacci_updated[j]);
		}

		// update the two last added vertices
		s = t;
		t = A[i];
	}

	// get cut weight
	cut = sum(n_max, adjMat[t]);

	// std::cout << "s=" << s << std::endl;
	// std::cout << "t=" << t << std::endl;
	// std::cout << "cut=" << cut << std::endl;

	// std::cout << "before merge:" << std::endl;
	// for (size_t i=0; i<n_max; i++) {
	// 	for (size_t j=0; j<n_max; j++) {
	// 		std::cout << adjMat[i][j] << "\t";
	// 	}
	// 	std::cout << std::endl;
	// }

	// generate cut
	std::list<_id_t> s2 = vertices.at(t);
	std::list<_id_t> s1;

	// get all vertices of the first sub-graph
	for (_id_t i=0; i<n_max; i++)
		if (i != t && vertices.count(i) > 0)
			for (_id_t j : vertices.at(i))
				s1.emplace_back(j);

	// merge s and t
	for (std::pair<_id_t,std::list<_id_t>> _t : vertices) {
		if (_t.first == t) {
			// add all of t's merged vertices to s
			vertices.at(s).insert(vertices.at(s).begin(), 
				_t.second.begin(), _t.second.end());
			// remove t
			vertices.erase(_t.first);
			break;
		}
	}

	// update adjMat by merging the edges of t to s
	for (size_t i=0; i<n_max; i++) {
		for (size_t j=0; j<n_max; j++) {
			if (i!=j) {
				if (i == s) {
					// std::cout << i << ":" << j << "= " << adjMat[i][j] << " with " 
					// 	<< t << ":" << j << "= " << adjMat[t][j] << std::endl;
					adjMat[i][j] += adjMat[t][j];
					adjMat[j][i] += adjMat[t][j];
				}
			}
		}
	}

	// remove t edges
	for (size_t i=0; i<n_max; i++) {
		for (size_t j=0; j<n_max; j++) {
			if (i == t || j == t) {
				// std::cout << i << ":" << j << "= " << adjMat[i][j] << std::endl;
				adjMat[i][j] = 0;
				adjMat[j][i] = 0;
			}
		}
	}

	// std::cout << std::endl << "after merge:" << std::endl;
	// for (size_t i=0; i<n_max; i++) {
	// 	for (size_t j=0; j<n_max; j++) {
	// 		std::cout << adjMat[i][j] << "\t";
	// 	}
	// 	std::cout << std::endl;
	// }

	return _cut(cut, s1, s2);
}

weight_t sum(size_t n, weight_t* A) {
	weight_t sum = 0;
	for (int i=0; i<n; i++)
		sum+=A[i];
	return sum;
}

/**************************************************/
/**              Debugging Functions             **/
/**************************************************/

void pv(std::map<_id_t,std::list<_id_t>> vertices) {
	for (std::pair<_id_t,std::list<_id_t>> v : vertices) {
		std::cout << v.first << std::endl;
		for (_id_t id : v.second)
			std::cout << "\t" << id << std::endl;
	}
}

void pf(fibonacci q) {
	for (fibonacci::ordered_iterator i=q.ordered_begin(); i!=q.ordered_end(); i++) {
		std::cout << i->second << ":" << i->first << std::endl;
	}
}