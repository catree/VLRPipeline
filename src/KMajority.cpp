/*
 * KMajority.cpp
 *
 *  Created on: Aug 28, 2013
 *      Author: andresf
 */

#include <KMajority.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/flann/random.h>
#include <opencv2/flann/dist.h>

#include <iostream>
#include <functional>
#include <bitset>
#include <ctime>

typedef cvflann::Hamming<uchar> Distance;
typedef typename Distance::ResultType DistanceType;

void KMajority::cluster(std::vector<cv::KeyPoint>& keypoints,
		const cv::Mat& descriptors) {

	if (descriptors.type() != CV_8U) {
		fprintf(stderr,
				"KMajority::cluster: error, descriptors matrix is not binary");
		return;
	}

	this->n = descriptors.rows;
	this->dim = descriptors.cols;

	this->belongs_to = new unsigned int[this->n];
	// Initially all transactions belong to any cluster
	std::fill_n(this->belongs_to, this->n, this->k);

	this->distance_to = new unsigned int[this->n];
	// Initially all transactions are at the farthest possible distance
	// i.e. dim*8 the max hamming distance
	std::fill_n(this->distance_to, this->n, this->dim * 8);

	this->cluster_counts = new unsigned int[this->k];
	// Initially no transaction is assigned to any cluster
	std::fill_n(this->cluster_counts, this->k, 0);

	// Randomly generate clusters
	this->initCentroids(descriptors);

	// Assign data to clusters
	this->quantize(keypoints, descriptors);

	bool converged = false;

	unsigned int iteration = 0;
	while (converged == false && iteration < max_iterations) {
		iteration++;
		// Compute the new cluster centers
		this->computeCentroids(keypoints, descriptors);

		// Reassign data to clusters
		converged = this->quantize(keypoints, descriptors);

		// Compute number of assigned transaction to each cluster
//		unsigned int* cluster_counts = new unsigned int[k];
//		std::fill_n(cluster_counts, k, 0);
//		for (unsigned int i = 0; i < n; i++) {
//			cluster_counts[belongs_to[i]]++;
//		}
		// TODO handle empty clusters case
		// Find empty clusters
//		for (unsigned int j = 0; j < k; j++) {
//			if (cluster_counts[j] == 0) {
//				printf("Cluster %u is empty\n", j);
//				// Find farthest element to its assigned cluster
//				unsigned int farthest_element_idx = 0;
//				for (unsigned int i = 1; i < n; i++) {
//					if (distance_to[i] > distance_to[farthest_element_idx]) {
//						farthest_element_idx = i;
//					}
//				}
//				// Re-assign farthest_element to the found empty cluster
//				cluster_counts[belongs_to[farthest_element_idx]]--;
//				belongs_to[farthest_element_idx] = j;
//				cluster_counts[j]++;
//				keypoints[farthest_element_idx].class_id = j;
//				distance_to[farthest_element_idx] = hammingDistance(
//						descriptors.row(farthest_element_idx),
//						centroids.row(j));
//			}
//		}
	}

}

void chooseCentersRandom(int k, int* indices, int indices_length, int* centers,
		int& centers_length) {
	cvflann::UniqueRandom r(indices_length);

	int index;
	for (index = 0; index < k; ++index) {
		bool duplicate = true;
		int rnd;
		while (duplicate) {
			duplicate = false;
			rnd = r.next();
			if (rnd < 0) {
				centers_length = index;
				return;
			}

			centers[index] = indices[rnd];

//			for (int j = 0; j < index; ++j) {
//				DistanceType sq = distance_(dataset_[centers[index]],
//						dataset_[centers[j]], dataset_.cols);
//				if (sq < 1e-16) {
//					duplicate = true;
//				}
//			}
		}
	}

	centers_length = index;
}

void KMajority::initCentroids(const cv::Mat& descriptors) {

	// Initializing variables useful for obtaining indexes of random chosen center
	cv::Ptr<int> centers_idx = new int[k];
	int centers_length;
	cv::Ptr<int> indices = new int[n];
	for (unsigned int i = 0; i < this->n; i++) {
		indices[i] = i;
	}

	// Randomly chose centers
	std::srand(unsigned(std::time(0)));
	chooseCentersRandom(k, indices, n, centers_idx, centers_length);

	// Assign centers based on the chosen indexes
	centroids.create(centers_length, dim, descriptors.type());
	for (int i = 0; i < centers_length; i++) {
		descriptors.row(centers_idx[i]).copyTo(
				centroids(cv::Range(i, i + 1), cv::Range(0, this->dim)));
	}

}

bool KMajority::quantize(std::vector<cv::KeyPoint>& keypoints,
		const cv::Mat& descriptors) {

	double mytime = cv::getTickCount();

	bool converged = true;

	// Comparison of all descriptors vs. all centroids
	for (unsigned int i = 0; i < this->n; i++) {
		// Set minimum distance as the distance to its assigned cluster
		int min_hd = distance_to[i];

		for (unsigned int j = 0; j < this->k; j++) {
			// Compute hamming distance between ith descriptor and jth cluster
			cv::Hamming distance;
			int hd = distance(descriptors.row(i).data, centroids.row(j).data,
					descriptors.cols);

			// Update cluster assignment to the nearest cluster
			if (hd < min_hd) {
				min_hd = hd;
				// If cluster assignment changed that means the algorithm hasn't converged yet
				if (belongs_to[i] != j) {
					converged = false;
				}
				// Decrease cluster count in case it was assigned to some valid cluster before
				// Recall that initially all transaction are assigned to kth cluster which
				// is not valid since valid clusters run from 0 to k-1
				if (belongs_to[i] != k) {
					cluster_counts[belongs_to[i]]--;
				}
				belongs_to[i] = j;
				cluster_counts[j]++;
				distance_to[i] = hd;
				keypoints[i].class_id = j;
			}
		}
	}

	mytime = ((double) cv::getTickCount() - mytime) / cv::getTickFrequency()
			* 1000;
	printf("   Method KMajority::quantize executed in %lf ms\n", mytime);
	return converged;
}

void KMajority::computeCentroids(const std::vector<cv::KeyPoint>& keypoints,
		const cv::Mat& descriptors) {

	cv::Mat bitwiseCount(1, this->dim * 8, cv::DataType<float>::type);
	// Loop over all clusters
	for (unsigned int j = 0; j < k; j++) {
		// Zeroing all cumulative variable dimension
		bitwiseCount(cv::Range(0, 1), cv::Range(0, bitwiseCount.cols)) =
				cv::Scalar::all(0);
		// Zero all the centroid dimensions
		centroids(cv::Range(j, j + 1), cv::Range(0, centroids.cols)) =
				cv::Scalar::all(0);
		// Loop over all data
		for (unsigned int i = 0; i < this->n; i++) {
			// Finding all data assigned to jth clusther
			if (belongs_to[i] == j) {
				//double mytime = cv::getTickCount();
				// TODO Check if there is any speed-up by processing all byte at once
				for (int l = 0; l < bitwiseCount.cols; l++) {
					// bit: 7-(l%8) col: (int)k/8 descriptor: i
					uchar byte = *(descriptors.row(i).col((int) l / 8).data);
					bitwiseCount.at<float>(0, l) += (int) ((byte
							>> (7 - (l % 8))) % 2);
				}
				//mytime = ((double) cv::getTickCount() - mytime) / cv::getTickFrequency() * 1000;
				//printf("   Method KMajority::computeCentroids processed descriptor in %lf ms\n", mytime);
			}
		}
		// In this point I already have stored in bitwiseCount the bitwise sum of all data assigned to jth cluster
		for (int l = 0; l < bitwiseCount.cols; l++) {
			/**
			 * If the bitcount for jth cluster at dimension l is greater than half of the data assigned to it
			 * then set bit to 1 otherwise set 0 (implicitly choosing 0 in case of ties)
			 */
			bool bit = bitwiseCount.at<float>(0, l)
					> (int) this->cluster_counts[j] / 2;
			centroids.at<unsigned char>(j, l) += (bit)
					<< ((bitwiseCount.cols - 1 - l) % 8);
		}
	}
}