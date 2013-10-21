/***********************************************************************
 *
 * bin_hierarchical_clustering_index.h
 *
 *  Created on: Sep 18, 2013
 *      Author: andresf
 *
 ***********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 * Copyright 2008-2009  Marius Muja (mariusm@cs.ubc.ca). All rights reserved.
 * Copyright 2008-2009  David G. Lowe (lowe@cs.ubc.ca). All rights reserved.
 *
 * THE BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************/

#ifndef BIN_HIERARCHICAL_CLUSTERING_INDEX_H_
#define BIN_HIERARCHICAL_CLUSTERING_INDEX_H_

#include <opencv2/flann/flann.hpp>

#include <CentersChooser.h>
#include <KMajorityIndex.h>

namespace cvflann {

enum WeightingType {
	TF_IDF, BINARY
};

struct VocabTreeParams: public IndexParams {
	VocabTreeParams(int branching = 10, int depth = 6, int iterations = 11,
			flann_centers_init_t centers_init = FLANN_CENTERS_RANDOM) {
		// branching factor
		(*this)["branching"] = branching;
		// max iterations to perform in one kmeans clustering (kmeans tree)
		(*this)["iterations"] = iterations;
		// tree depth
		(*this)["depth"] = depth;
		// algorithm used for picking the initial cluster centers for kmeans tree
		(*this)["centers_init"] = centers_init;
	}
};

class ImageCount {
public:
	ImageCount() :
			m_index(0), m_count(0.0) {
	}
	ImageCount(unsigned int index, float count) :
			m_index(index), m_count(count) {
	}

	// Index of the database image this entry corresponds to
	unsigned int m_index;
	// (Weighted, normalized) Count of how many times this feature appears
	float m_count;
};

template<class TDescriptor, class Distance = cv::flann::Hamming<TDescriptor> >
class VocabTree {

private:

	typedef typename Distance::ElementType ElementType;
	typedef typename Distance::ResultType DistanceType;

	/**
	 * Structure representing a node in the hierarchical k-means tree.
	 */
	struct VocabTreeNode {
		// The cluster center
		TDescriptor* center;
		// Children nodes (only for non-terminal nodes)
		// Note: no need to store how many children does it has because
		// this is a k-ary tree, where 'k' is the branch factor, that is has k children or none
		VocabTreeNode** children;
		// Word id (only for terminal nodes)
		// TODO Check if this attribute is truly necessary
		int word_id;
		// Weight (only for terminal nodes)
		double weight;
		// Inverse document/image list (only for terminal nodes)
		std::vector<ImageCount> image_list;
		VocabTreeNode() :
				center(NULL), children(NULL), word_id(-1), weight(0.0) {
			image_list.clear();
		}
		VocabTreeNode& operator=(const VocabTreeNode& node) {
			center = node.center;
			children = node.children;
			word_id = node.word_id;
			weight = node.weight;
			return *this;
		}
	};

	typedef VocabTreeNode* VocabTreeNodePtr;

protected:

	/* Attributes useful for clustering */
	// The function used for choosing the cluster centers
	cvflann::flann_centers_init_t m_centers_init;
	// Maximum number of iterations to use when performing k-means clustering
	int m_iterations;
	// The dataset used by this index
	const cv::Mat& m_dataset;

	/* Attributes useful for describing the tree */
	// The branching factor used in the hierarchical k-means clustering
	int m_branching;
	// Depth levels
	int m_depth;
	// Length of each feature.
	size_t m_veclen;

	/* Attributes actually holding the tree */
	// The root node in the tree.
	VocabTreeNodePtr m_root;
	// Words of the vocabulary
	std::vector<VocabTreeNodePtr> m_words;

	/* Attributes used by several methods */
	// The distance
	Distance m_distance;
	// Memory occupied by the index
	int m_memoryCounter;

public:

	/**
	 * Tree constructor
	 *
	 * @param params - Parameters passed to the binary hierarchical k-means algorithm
	 */
	VocabTree(const cv::Mat& inputData = cv::Mat(), const IndexParams& params =
			VocabTreeParams());

	/**
	 * Tree destructor, releases the memory used by the tree.
	 */
	virtual ~VocabTree();

	/**
	 * Returns the size of the tree.
	 *
	 * @return number of leaf nodes in the tree
	 */
	size_t size();

	/**
	 * Builds the tree.
	 *
	 * @param inputData - Matrix with the data to be clustered
	 *
	 * @note After this method is executed m_root holds a pointer to the tree,
	 *		 while m_words holds pointers to the leaf nodes.
	 * @note Interior nodes have only 'center' and 'children' information,
	 * 		 while leaf nodes have only 'center' and 'word_id', all weights for
	 * 		 interior nodes are 0 while weights for leaf nodes are 1.
	 */
	void build();

	/**
	 * Saves the tree to a stream.
	 *
	 * @param stream - The stream to save the tree to
	 */
	void save(const std::string& filename) const;

	/**
	 * Loads the tree from a stream.
	 *
	 * @param stream - The stream from which the tree is loaded
	 */
	void load(const std::string& filename);

	/**
	 * Pushes DB image features down the tree until a leaf node,
	 * once reached updates the inverted file.
	 *
	 * @param imgIdx - The id of the image
	 * @param imgFeatures - Matrix of features representing the image
	 */
	void addImageToDatabase(uint imgIdx, cv::Mat dbImgFeatures);

	/**
	 * Using the pre-loaded inverted files assigns words weights
	 * according to the chosen weighting scheme.
	 *
	 * @param NWords
	 * @param weighting - The weighting scheme to apply
	 */
	void computeWordsWeights(WeightingType weighting,
			const uint numDbWords = 0);

	/**
	 * Computes the DB BoW vectors by applying the words weights
	 * to the image counts in the inverted files.
	 *
	 * @note Might be better not computing the DB BoW vectors in advance
	 *		 but simply holding the histogram counts and obtaining
	 *		 the score by component-wise weighting and scoring (DRY way)
	 */
	void createDatabase();

	/**
	 * Normalizes the DB BoW vectors by dividing the weighted counts
	 * stored in the inverted files.
	 *
	 * @param numDbImages
	 * @param normType
	 */
	void normalizeDatabase(const uint numDbImages, int normType = cv::NORM_L1);

	/**
	 * Clears the inverted files from the leaf nodes
	 */
	void clearDatabase();

	/**
	 * Computes the query BoW vector of an image by pushing down the tree the query image
	 * features and applying the words weights, followed by efficiently scoring it against
	 * the pre-computed DB BoW vectors.
	 *
	 * @param queryImgFeatures - Matrix containing the features of the query image
	 * @param numDbImages - Number of DB images, used for creating the scores matrix
	 * @param scores - Row matrix of size [1 x n] where n is the number DB images
	 * @param scoringMethod - normalization method used for scoring BoW vectors
	 *
	 * @note DB BoW vectors must be normalized beforehand
	 */
	void scoreQuery(const cv::Mat& queryImgFeatures, cv::Mat& scores,
			const uint numDbImages, const int normType = cv::NORM_L2) const;

private:

	/**
	 * Recursively releases the memory allocated to store the tree node centers.
	 *
	 * @param node - A pointer to a node in the tree where to start the releasing
	 */
	void free_centers(VocabTreeNodePtr node);

	/**
	 * Computes the centroid of a node. (Only for the root node)
	 *
	 * @param node - The node to use
	 * @param indices - The array of indices of the points belonging to the node
	 * @param indices_length - The number of indices in the array of indices
	 */
	void computeNodeStatistics(VocabTreeNodePtr node, int* indices,
			int indices_length);

	/**
	 * The method responsible with actually doing the recursive hierarchical clustering.
	 *
	 * @param node - The node to cluster
	 * @param indices - Indices of the points belonging to the current node
	 * @param indices_length
	 */
	void computeClustering(VocabTreeNodePtr node, int* indices,
			int indices_length, int level);

	/**
	 * Saves the vocabulary tree starting at a given node to a stream.
	 *
	 * @param fs - A reference to the file storage pointing to the file where to save the tree
	 * @param node - The node indicating the root of the tree to save
	 */
	void save_tree(cv::FileStorage& fs, VocabTreeNodePtr node) const;

	/**
	 * Loads the vocabulary tree from a stream and stores into into a given node pointer.
	 *
	 * @param filename - A reference to the file storage where to read node parameters
	 * @param node - The node where to store the loaded tree
	 */
	void load_tree(cv::FileNode& fs, VocabTreeNodePtr& node);

	/**
	 * Quantizes a single feature vector into a word. Traverses the whole tree,
	 * and stores the resulting word id and weight.
	 *
	 * @param feature - Row vector representing the feature vector to quantize
	 * @param id - The id of the found word
	 * @param weight - The weight of the found word
	 */
	void quantize(const cv::Mat& feature, uint &id, double &weight) const;

	/**
	 * Returns whether the vocabulary is empty (i.e. it has not been trained)
	 *
	 * @return true if and only if the vocabulary is empty
	 */
	bool empty() const;

	/**
	 * Updates the inverted file of the given word by adding the image indicated
	 * by the given imgIdx.
	 *
	 * @param wordIdx - The id of the word whose inverted file to update
	 * @param imgIdx - The id of the image to add to the inverted file
	 *
	 * @note Images are added in sequence
	 */
	void addFeatureToInvertedFile(uint wordIdx, uint imgIdx);

	/**
	 * Transforms a set of data (representing a single image) into a BoW vector.
	 *
	 * @param featuresVector - Matrix of data to quantize
	 * @param bowVector - BoW vector of weighted words
	 * @param normType - Norm used to normalize the output query BoW vector
	 */
	void transform(const cv::Mat& featuresVector, cv::Mat& bowVector,
			const int& normType) const;

};

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
VocabTree<TDescriptor, Distance>::VocabTree(const cv::Mat& inputData,
		const IndexParams& params) :
		m_dataset(inputData), m_veclen(0), m_root(NULL), m_distance(Distance()), m_memoryCounter(
				0) {

	// Attributes initialization
	m_veclen = m_dataset.cols;
	m_branching = get_param(params, "branching", 6);
	m_iterations = get_param(params, "iterations", 11);
	m_depth = get_param(params, "depth", 10);
	m_centers_init = get_param(params, "centers_init", FLANN_CENTERS_RANDOM);

	if (m_iterations < 0) {
		m_iterations = (std::numeric_limits<int>::max)();
	}

	m_words.clear();
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
VocabTree<TDescriptor, Distance>::~VocabTree() {
	if (m_root != NULL) {
		free_centers(m_root);
	}
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::free_centers(VocabTreeNodePtr node) {
	delete[] node->center;
	if (node->children != NULL) {
		for (int k = 0; k < m_branching; ++k) {
			free_centers(node->children[k]);
		}
	}
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
size_t VocabTree<TDescriptor, Distance>::size() {
	return m_words.size();
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::build() {

	if (m_branching < 2) {
		throw std::runtime_error("[VocabTree::build] Error, branching factor"
				" must be at least 2");
	}

	// Number of features in the dataset
	size_t size = m_dataset.rows;

	//  Array of indices to vectors in the dataset
	int* indices = new int[size];
	for (size_t i = 0; i < size; ++i) {
		indices[i] = int(i);
	}

	m_root = new VocabTreeNode();
	computeNodeStatistics(m_root, indices, (int) size);

	printf("[VocabTree::build] Started clustering\n");

	computeClustering(m_root, indices, (int) size, 0);

	printf("[VocabTree::build] Finished clustering\n");

}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::save(const std::string& filename) const {

	cv::FileStorage fs(filename.c_str(), cv::FileStorage::WRITE);

	if (!fs.isOpened()) {
		throw std::runtime_error(
				"[VocabTree::save] Error opening file " + filename
						+ " for writing");
	}

	fs << "iterations" << m_iterations;
	fs << "branching" << m_branching;
	fs << "depth" << m_depth;
	fs << "vectorLength" << (int) m_veclen;
	fs << "memoryCounter" << m_memoryCounter;

	fs << "root";

	save_tree(fs, m_root);

	fs.release();
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::save_tree(cv::FileStorage& fs,
		VocabTreeNodePtr node) const {

	// WriteNode
	fs << "{";
	fs << "center" << cv::Mat(1, m_veclen, m_dataset.type(), node->center);
	fs << "weight" << node->weight;
	fs << "wordId" << node->word_id;

	fs << "children" << "[";
	if (node->children != NULL) {
		// WriteChildren
		for (size_t i = 0; (int) i < m_branching; i++) {
			save_tree(fs, node->children[i]);
		}
	}
	fs << "]";

	fs << "imageList" << "[";
	for (ImageCount img : node->image_list) {
		fs << "{:" << "m_index" << (int) img.m_index << "m_count" << img.m_count
				<< "}";
	}
	fs << "]";

	fs << "}";
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::load(const std::string& filename) {

	cv::FileStorage fs(filename.c_str(), cv::FileStorage::READ);

	if (!fs.isOpened()) {
		throw std::runtime_error(
				std::string("Could not open file ") + filename);
	}

	m_iterations = (int) fs["iterations"];
	m_branching = (int) fs["branching"];
	m_depth = (int) fs["depth"];
	m_veclen = (int) fs["vectorLength"];
	m_memoryCounter = (int) fs["memoryCounter"];

	cv::FileNode root = fs["root"];

	m_root = new VocabTreeNode();

	load_tree(root, m_root);

	fs.release();
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::load_tree(cv::FileNode& fs,
		VocabTreeNodePtr& node) {

	cv::Mat center;
	fs["center"] >> center;
	CV_Assert(center.rows == 1);
	CV_Assert(center.cols == (int ) m_veclen);

	// Deep copy
	node->center = new TDescriptor[m_veclen];
	m_memoryCounter += (int) (m_veclen * sizeof(TDescriptor));
	for (size_t k = 0; k < m_veclen; ++k) {
		node->center[k] = center.at<TDescriptor>(0, k);
	}

	node->weight = (double) fs["weight"];
	node->word_id = (int) fs["wordId"];

	cv::FileNode children = fs["children"];

	if (children.size() == 0) {
		// Node has no children then it's a leaf node
		cv::FileNode images = fs["imageList"];

		// Verifying that imageList is a sequence
		if (children.type() != cv::FileNode::NONE
				&& images.type() != cv::FileNode::SEQ) {
			std::stringstream ss;
			ss << "Error while parsing tree, fetched element"
					" 'images' should be a sequence";
			throw std::runtime_error(ss.str());
		}

		node->children = NULL;
		node->image_list.clear();
		for (cv::FileNodeIterator it = images.begin(); it != images.end();
				++it) {
			size_t index = (int) (*it)["m_index"];
			ImageCount* img = new ImageCount(index, (float) (*it)["m_count"]);
			node->image_list.push_back(*img);
		}

		m_words.push_back(node);

	} else {
		// Node has children then it's an interior node

		// Verifying that children is a sequence
		if (children.type() != cv::FileNode::NONE
				&& children.type() != cv::FileNode::SEQ) {
			std::stringstream ss;
			ss << "Error while parsing tree, fetched element"
					" 'children' should be a sequence";
			throw std::runtime_error(ss.str());
		}

		// Verifying that children has 0 or k elements
		if (children.size() != 0 && (int) children.size() != m_branching) {
			std::stringstream ss;
			ss << "Error while parsing tree, fetched element"
					" 'children' must have [0] or [" << m_branching
					<< "] elements";
			throw std::runtime_error(ss.str());
		}

		node->children = new VocabTreeNodePtr[m_branching];
		cv::FileNodeIterator it = children.begin();

		for (size_t c = 0; (int) c < m_branching; ++c, it++) {
			node->children[c] = new VocabTreeNode();
			cv::FileNode child = *it;
			load_tree(child, node->children[c]);
		}
	}

}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::computeNodeStatistics(
		VocabTreeNodePtr node, int* indices, int indices_length) {

	TDescriptor* center = new TDescriptor[m_veclen];

	m_memoryCounter += int(m_veclen * sizeof(TDescriptor));

	// Compute center using majority voting over all data
	cv::Mat accVector(1, m_veclen * 8, cv::DataType<int>::type);
	accVector = cv::Scalar::all(0);
	for (size_t i = 0; (int) i < indices_length; i++) {
		KMajorityIndex::cumBitSum(m_dataset.row(indices[i]), accVector);
	}
	cv::Mat centroid(1, m_veclen, m_dataset.type());
	KMajorityIndex::majorityVoting(accVector, centroid, indices_length);

	for (size_t k = 0; k < m_veclen; ++k) {
		center[k] = centroid.at<TDescriptor>(0, k);
	}

	node->center = center;
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::computeClustering(VocabTreeNodePtr node,
		int* indices, int indices_length, int level) {

	// Recursion base case: done when the last level is reached
	// or when there are less data than clusters
	if (level == m_depth - 1 || indices_length < m_branching) {
//		std::sort(node->indices, node->indices + indices_length);
		node->children = NULL;
		node->word_id = m_words.size();
		node->weight = 1.0;
		this->m_words.push_back(node);
		return;
	}

	printf(
			"[VocabTree::computeClustering] (level %d): Running k-means (%d features)\n",
			level, indices_length);

	int* centers_idx = new int[m_branching];
	int centers_length;

	CentersChooser<Distance>::create(m_centers_init)->chooseCenters(m_branching,
			indices, indices_length, centers_idx, centers_length, m_dataset);

	// Recursion base case: done as well if by case got
	// less cluster indices than clusters
	if (centers_length < m_branching) {
//		std::sort(node->indices, node->indices + indices_length);
		node->children = NULL;
		node->word_id = m_words.size();
		node->weight = 1.0;
		this->m_words.push_back(node);
		delete[] centers_idx;
		return;
	}

	// TODO initCentroids: assign centers based on the chosen indexes

//	printf("[BuildRecurse] (level %d): initCentroids - assign centers based on the chosen indexes\n", level);

	cv::Mat dcenters(m_branching, m_veclen, m_dataset.type());
	for (int i = 0; i < centers_length; i++) {
		m_dataset.row(centers_idx[i]).copyTo(
				dcenters(cv::Range(i, i + 1), cv::Range(0, m_veclen)));
	}
	delete[] centers_idx;

	int* count = new int[m_branching];
	for (int i = 0; i < m_branching; ++i) {
		count[i] = 0;
	}

	//TODO quantize: assign points to clusters

	int* belongs_to = new int[indices_length];
	for (int i = 0; i < indices_length; ++i) {

		DistanceType sq_dist = m_distance(m_dataset.row(indices[i]).data,
				dcenters.row(0).data, m_veclen);
		belongs_to[i] = 0;
		for (int j = 1; j < m_branching; ++j) {
			DistanceType new_sq_dist = m_distance(
					m_dataset.row(indices[i]).data, dcenters.row(j).data,
					m_veclen);
			if (sq_dist > new_sq_dist) {
				belongs_to[i] = j;
				sq_dist = new_sq_dist;
			}
		}
		count[belongs_to[i]]++;
	}

	bool converged = false;
	int iteration = 0;
	while (!converged && iteration < m_iterations) {
		converged = true;
		iteration++;

		// TODO: computeCentroids compute the new cluster centers
		// Warning: using matrix of integers, there might be
		// an overflow when summing too much descriptors
		cv::Mat bitwiseCount(m_branching, m_veclen * 8,
				cv::DataType<int>::type);
		// Zeroing matrix of cumulative bits
		bitwiseCount = cv::Scalar::all(0);
		// Zeroing all the centroids dimensions
		dcenters = cv::Scalar::all(0);

		// Bitwise summing the data into each centroid
		for (size_t i = 0; (int) i < indices_length; i++) {
			uint j = belongs_to[i];
			cv::Mat b = bitwiseCount.row(j);
			KMajorityIndex::cumBitSum(m_dataset.row(indices[i]), b);
		}
		// Bitwise majority voting
		for (size_t j = 0; (int) j < m_branching; j++) {
			cv::Mat centroid = dcenters.row(j);
			KMajorityIndex::majorityVoting(bitwiseCount.row(j), centroid,
					count[j]);
		}

		// TODO quantize: reassign points to clusters
		for (int i = 0; i < indices_length; ++i) {
			DistanceType sq_dist = m_distance(m_dataset.row(indices[i]).data,
					dcenters.row(0).data, m_veclen);
			int new_centroid = 0;
			for (int j = 1; j < m_branching; ++j) {
				DistanceType new_sq_dist = m_distance(
						m_dataset.row(indices[i]).data, dcenters.row(j).data,
						m_veclen);
				if (sq_dist > new_sq_dist) {
					new_centroid = j;
					sq_dist = new_sq_dist;
				}
			}
			if (new_centroid != belongs_to[i]) {
				count[belongs_to[i]]--;
				count[new_centroid]++;
				belongs_to[i] = new_centroid;

				converged = false;
			}
		}

		// TODO handle empty clusters
		for (int i = 0; i < m_branching; ++i) {
			// if one cluster converges to an empty cluster,
			// move an element into that cluster
			if (count[i] == 0) {
				int j = (i + 1) % m_branching;
				while (count[j] <= 1) {
					j = (j + 1) % m_branching;
				}

				for (int k = 0; k < indices_length; ++k) {
					if (belongs_to[k] == j) {
						belongs_to[k] = i;
						count[j]--;
						count[i]++;
						break;
					}
				}
				converged = false;
			}
		}

	}

	TDescriptor** centers = new TDescriptor*[m_branching];

	for (int i = 0; i < m_branching; ++i) {
		centers[i] = new TDescriptor[m_veclen];
		m_memoryCounter += (int) (m_veclen * sizeof(TDescriptor));
		for (size_t k = 0; k < m_veclen; ++k) {
			centers[i][k] = dcenters.at<TDescriptor>(i, k);
		}
	}

	// Compute k-means clustering for each of the resulting clusters
	node->children = new VocabTreeNodePtr[m_branching];
	int start = 0;
	int end = start;
	for (int c = 0; c < m_branching; ++c) {
		// Re-order indices by chunks in clustering order
		for (int i = 0; i < indices_length; ++i) {
			if (belongs_to[i] == c) {
				std::swap(indices[i], indices[end]);
				std::swap(belongs_to[i], belongs_to[end]);
				end++;
			}
		}

		node->children[c] = new VocabTreeNode();
		node->children[c]->center = centers[c];
		computeClustering(node->children[c], indices + start, end - start,
				level + 1);
		start = end;
	}

	dcenters.release();
	delete[] centers;
	delete[] count;
	delete[] belongs_to;
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::transform(const cv::Mat& featuresVector,
		cv::Mat& bowVector, const int& normType) const {

	// Initialize query BoW vector
	bowVector = cv::Mat::zeros(1, m_words.size(), cv::DataType<float>::type);

	// Quantize each query image feature vector
	for (size_t i = 0; (int) i < featuresVector.rows; i++) {
		uint wordIdx;
		double wordWeight;
		quantize(featuresVector.row(i), wordIdx, wordWeight);

		if (wordIdx > m_words.size() - 1 || wordIdx < 0) {
			throw std::runtime_error(
					"[VocabTree::scoreQuery] Feature quantized into a non-existent word");
		}

		bowVector.at<float>(0, wordIdx) += (float) wordWeight;
	}

	//	Normalizing query BoW vector
	cv::normalize(bowVector, bowVector, 1, 0, normType);
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::quantize(const cv::Mat& feature,
		uint &word_id, double &weight) const {

	VocabTreeNodePtr best_node = m_root;

	while (best_node->children != NULL) {

		VocabTreeNodePtr node = best_node;

		// Arbitrarily assign to first child
		best_node = node->children[0];
		DistanceType best_distance = m_distance(feature.data, best_node->center,
				m_veclen);

		// Looking for a better child
		for (size_t j = 1; (int) j < this->m_branching; j++) {
			DistanceType d = m_distance(feature.data, node->children[j]->center,
					m_veclen);
			if (d < best_distance) {
				best_distance = d;
				best_node = node->children[j];
			}
		}
	}

	// Turn node id into word id
	word_id = best_node->word_id;
	weight = best_node->weight;
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::computeWordsWeights(
		WeightingType weighting, const uint numDbWords) {
	if (weighting == cvflann::BINARY) {
		// Setting constant weight equal to 1
		for (VocabTreeNodePtr& word : m_words) {
			word->weight = 1.0;
		}
	} else if (weighting == cvflann::TF_IDF) {
		// Calculating the IDF part of the TF-IDF score, the complete
		// TF-IDF score is the result of multiplying the weight by the word count
		for (VocabTreeNodePtr& word : m_words) {
			int len = word->image_list.size();
			// TODO Check that descriptors are being correctly quantized
			// because having that a descriptor from all DB images is quantized
			// to the same word is quite unlikely
			if (len > 0) {
				word->weight = log((double) numDbWords / (double) len);
			} else {
				word->weight = 0.0;
			}
		}
	} else {
		throw std::runtime_error(
				"[VocabTree::computeWordsWeights] Unknown weighting type");
	}
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::createDatabase() {

	// Loop over words
	for (VocabTreeNodePtr& word : m_words) {
		// Apply word weight to the image count
		for (ImageCount& image : word->image_list) {
			image.m_count *= word->weight;
		}
	}

}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::clearDatabase() {
	for (VocabTreeNodePtr& word : m_words) {
		word->image_list.clear();
	}
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
bool VocabTree<TDescriptor, Distance>::empty() const {
	return m_words.empty();
}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::addImageToDatabase(uint imgIdx,
		cv::Mat imgFeatures) {

	if (imgFeatures.type() != CV_8U) {
		throw std::runtime_error(
				"[VocabTree::addImageToDatabase] Features matrix is not binary");
	}

	if (imgFeatures.cols != (int) m_veclen) {
		std::stringstream ss;
		ss << "[VocabTree::addImageToDatabase] Features vectors must be "
				<< m_veclen << " bytes long, i.e. " << m_veclen * 8
				<< "-dimensional";
		throw std::runtime_error(ss.str());
	}

	if (imgFeatures.rows < 1) {
		throw std::runtime_error(
				"[VocabTree::addImageToDatabase] At least one feature vector is needed");
	}

	if (empty()) {
		throw std::runtime_error(
				"[VocabTree::addImageToDatabase] Vocabulary is empty");
	}

	for (size_t i = 0; (int) i < imgFeatures.rows; i++) {
		uint wordIdx;
		double wordWeight; // not needed
		// w is the IDF value if TF_IDF, 1 if TF
		// w is the Inverse Document Frequency if IDF, 1 if BINARY
		quantize(imgFeatures.row(i), wordIdx, wordWeight);
		addFeatureToInvertedFile(wordIdx, imgIdx);
	}

}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::addFeatureToInvertedFile(uint wordIdx,
		uint imgIdx) {

	int n = (int) m_words[wordIdx]->image_list.size();

	// Images list is empty: push a new image
	if (n == 0) {
		m_words[wordIdx]->image_list.push_back(ImageCount(imgIdx, (float) 1.0));
	} else {
		// Images list is not empty: check if the id of the last added image
		// is the same than that of the image being added
		if (m_words[wordIdx]->image_list[n - 1].m_index == imgIdx) {
			// Images are equal then the counter is increased by one
			m_words[wordIdx]->image_list[n - 1].m_count += (float) 1.0;
		} else {
			// Images are different then push a new image
			m_words[wordIdx]->image_list.push_back(
					ImageCount(imgIdx, (float) 1.0));
		}
	}

}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::normalizeDatabase(
		const uint num_db_images, int normType) {

	// Magnitude of a vector is defined as: sum(abs(xi)^p)^(1/p)

	std::vector<float> mags(num_db_images, 0.0);

	// Computing DB BoW vectors magnitude

	// Summing vector elements
	for (VocabTreeNodePtr& word : m_words) {
		for (ImageCount& image : word->image_list) {
			uint index = image.m_index;
			double dim = image.m_count;

			assert(index < mags.size());

			if (normType == cv::NORM_L1) {
				mags[index] += fabs(dim);
			} else if (normType == cv::NORM_L2) {
				mags[index] += pow(dim, 2);
			} else {
				throw std::runtime_error(
						"[VocabTree::scoreQuery] Unknown scoring method");
			}
		}
	}

	// Applying power over sum result
	if (normType == cv::NORM_L2) {
		for (size_t i = 0; i < num_db_images; i++) {
			mags[i] = sqrt(mags[i]);
		}
	}

	// Normalizing database
	for (VocabTreeNodePtr& word : m_words) {
		for (ImageCount& image : word->image_list) {
			uint index = image.m_index;
			assert(index < mags.size());
			if (mags[index] > 0.0) {
				image.m_count /= mags[index];
			}
		}
	}

}

// --------------------------------------------------------------------------

template<class TDescriptor, class Distance>
void VocabTree<TDescriptor, Distance>::scoreQuery(
		const cv::Mat& queryImgFeatures, cv::Mat& scores,
		const uint numDbImages, const int normType) const {

	if (queryImgFeatures.type() != CV_8U) {
		throw std::runtime_error(
				"[VocabTree::scoreQueryFeatures] Features matrix is not binary");
	}

	if (queryImgFeatures.cols != (int) m_veclen) {
		std::stringstream ss;
		ss << "[VocabTree::scoreQueryFeatures] Features vectors must be "
				<< m_veclen << " bytes long, i.e. " << m_veclen * 8
				<< "-dimensional";
		throw std::runtime_error(ss.str());
	}

	if (queryImgFeatures.rows < 1) {
		throw std::runtime_error(
				"[VocabTree::scoreQueryFeatures] At least one feature vector is needed");
	}

	if (empty()) {
		throw std::runtime_error(
				"[VocabTree::scoreQueryFeatures] Vocabulary is empty");
	}

	if (normType != cv::NORM_L1 && normType != cv::NORM_L2) {
		throw std::runtime_error(
				"[VocabTree::scoreQuery] Unknown scoring method");
	}

	scores = cv::Mat::zeros(1, numDbImages, cv::DataType<float>::type);

	cv::Mat queryBowVector;
	transform(queryImgFeatures, queryBowVector, normType);

	//	Efficient scoring query BoW vector against all DB BoW vectors

	// ||v - w||_{L1} = 2 + Sum(|v_i - w_i| - |v_i| - |w_i|)
	// ||v - w||_{L2} = sqrt( 2 - 2 * Sum(v_i * w_i) )

	// Calculating sum part of the efficient score implementation
	for (VocabTreeNodePtr word : m_words) {
		float qi = queryBowVector.at<float>(0, word->word_id);

		// Early exit
		if (qi == 0.0) {
			continue;
		}

		// The inverted file of a word contains all images counts quantized into that word
		// i.e. if they are there its because their count di is not zero

		// In addition its fair computing qi against di without further verification
		// since the inverted files contain not null counts

		for (ImageCount& image : word->image_list) {
			float di = image.m_count;

			// qi cannot be zero because we are considering only when its non-zero
			// qi cannot be more than 1 because it is supposed to be normalized
			CV_Assert(qi > 0 && qi <= 1.0);

			// di cannot more than 1 because it is supposed to be normalized
			CV_Assert(di <= 1.0);

			// di cannot be zero (unless the weight is zero) because the inverted files
			// contain only counts for images with a descriptor which was quantized
			// into that word
			if (word->weight != 0.0) {
				CV_Assert(di > 0.0);
			} else {
				CV_Assert(di >= 0.0);
			}

			if (normType == cv::NORM_L1) {
				scores.at<float>(0, image.m_index) += (float) (fabs(qi - di)
						- fabs(qi) - fabs(di));
			} else if (normType == cv::NORM_L2) {
				scores.at<float>(0, image.m_index) += (float) qi * di;
			}
		}
	}

	// Completing efficient score implementation
	for (int i = 0; i < scores.cols; i++) {
		if (normType == cv::NORM_L1) {
			scores.at<float>(0, i) = (float) (-scores.at<float>(0, i) / 2.0);
		} else if (normType == cv::NORM_L2) {
			scores.at<float>(0, i) =
					(float) (2.0 - 2.0 * scores.at<float>(0, i));
		}
		// else, not possible since normType was validated before
	}

}

} /* namespace cvflann */

#endif /* BIN_HIERARCHICAL_CLUSTERING_INDEX_H_ */