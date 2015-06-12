/*
 * CachePolicy.h
 *
 *  Created on: Aug 26, 2014
 *      Author: george
 */

#ifndef CACHEPOLICY_H_
#define CACHEPOLICY_H_

#include <list>
#include <string>
#include <iostream>


class DRKey{
private:
	std::string rtName;
	std::string rtId;
	std::string drName;
	std::string drId;
	int timeStamp;
	int version;

public:
	DRKey();
	DRKey(std::string rtName, std::string rtId, std::string drName, std::string drId, int timeStamp, int version);
	virtual ~DRKey();
	bool isEqual(DRKey element);

	std::string getDrName() const;
	void setDrName(std::string drName);
	std::string getRtId() const;
	void setRtId(std::string rtId);
	std::string getRtName() const;
	void setRtName(std::string rtName);
	int getTimeStamp() const;
	void setTimeStamp(int timeStamp);
	int getVersion() const;
	void setVersion(int version);
	// debugging only method
	void print();
	std::string getDrId() const;
	void setDrId(std::string drI);
};

// this base class implements FIFO
class CachePolicy {
protected:
	std::list<DRKey> accessList;

public:
	CachePolicy();
	virtual ~CachePolicy();

	// item was inserted
	virtual void insertItem(DRKey id);
	// inform that item was accessed
	virtual void accessItem(DRKey id);
	// select item for delete
	virtual DRKey selectDelete(void);
	// cache policy container size
	virtual int size();


};

#endif /* CACHEPOLICY_H_ */
