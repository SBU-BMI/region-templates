#ifndef MIN_CUT_HPP
#define MIN_CUT_HPP

#include <list>
#include <boost/heap/fibonacci_heap.hpp>

/**************************************************/
/**                Basic Entities                **/
/**************************************************/

namespace mincut {

	// basic types
	typedef int _id_t;
	typedef int weight_t;

	typedef std::pair<weight_t, _id_t> node_t;
	inline node_t _node(weight_t w,_id_t id) {return std::make_pair(w,id);}

	typedef std::list<_id_t> subgraph_t;
	typedef std::pair<weight_t, std::pair<subgraph_t, subgraph_t>> cut_t;
	inline cut_t _cut(weight_t w, subgraph_t s1, subgraph_t s2) {return std::make_pair(w,std::make_pair(s1,s2));}
	inline weight_t _cut_w(cut_t c) {return c.first;}
	inline subgraph_t _cut_s1(cut_t c) {return c.second.first;}
	inline subgraph_t _cut_s2(cut_t c) {return c.second.second;}

	/**************************************************/
	/**             Fibonacci Comparator             **/
	/**************************************************/
	/** This is needed to enforce that the weight is **/
	/** ordered in a descending manner, and id is    **/
	/** ordered in a ascending manner on the         **/
	/** fibonacci heap.                              **/
	/**************************************************/

	struct my_comparator {
		bool operator() (const node_t& x, const node_t& y) const {
			if (x.first != y.first)
				return x.first < y.first;
			else
				return x.second > y.second;
		}
		typedef node_t first_argument_type;
		typedef node_t second_argument_type;
		typedef bool result_type;
	};

	typedef boost::heap::fibonacci_heap<node_t, 
		boost::heap::compare<my_comparator>> fibonacci;

	std::list<cut_t> min_cut(size_t n, weight_t** adjMat);

}
#endif