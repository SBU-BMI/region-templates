#ifndef PRIORITY_Q_H
#define PRIORITY_Q_H

// Thread-safe priority queue
template <typename T>
class PriorityQ {
public:
	PriorityQ();
	void push(T& e);
	// returns 0 if no element is returned
	int pop(T& e);
};

#include "PriorityQ.tpp"

#endif