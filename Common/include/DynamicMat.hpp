/*
 * DynamicMat.hpp
 *
 *  Created on: Nov 10, 2013
 *      Author: andresf
 */

#ifndef DYNAMICMAT_HPP_
#define DYNAMICMAT_HPP_

#include <map>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

struct image {
	int imgIdx; // Index of the image it represents
	int startIdx; // (inclusive) Starting index in the merged descriptors matrix
	image() :
			imgIdx(-1), startIdx(-1) {
	}
};

static std::vector<image> DEFAULT_INDICES;
static std::vector<std::string> DEFAULT_KEYS;

class DynamicMat {

public:

	DynamicMat();

	DynamicMat(std::vector<std::string>& keysFilenames);

	// Copy constructor
	DynamicMat(const DynamicMat& other);

	// Assignment operators
	DynamicMat& operator =(const DynamicMat& other);

	virtual ~DynamicMat();

	const std::vector<image>& getDescriptorsIndices() const {
		return m_descriptorsIndices;
	}

	const std::vector<std::string>& getKeysFilenames() const {
		return m_keysFilenames;
	}

	cv::Mat row(int descriptorIndex);

	int type() const;

	bool empty() const;

	/**
	 * Returns the count of memory used by the matrices loaded in cache.
	 *
	 * @return memory count in Bytes
	 */
	int getMemoryCount() const {
		return m_memoryCount;
	}

protected:

	std::vector<image> m_descriptorsIndices;
	std::vector<std::string> m_keysFilenames;
	std::map<int, cv::Mat> descriptorCache;
	std::queue<int> addingOrder;

	static int computeUsedMemory(cv::Mat& descriptors) {
		return int(descriptors.rows * descriptors.cols * descriptors.elemSize());
	}

public:
	int rows = 0;
	int cols = 0;

private:
	static const int MAX_MEM = 524288000; // ~500 MBytes
	int m_memoryCount = 0;
	int m_descriptorType = -1;

};

#endif /* DYNAMICMAT_HPP_ */