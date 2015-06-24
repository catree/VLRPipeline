/*
 * IncrementalKMeans_test.cpp
 *
 *  Created on: May 13, 2015
 *      Author: andresf
 */

#include <gtest/gtest.h>

#include <FileUtils.hpp>
#include <FunctionUtils.hpp>
#include <IncrementalKMeans.hpp>

namespace vlr {

TEST(IncrementalKMeans, InitWithFakeData) {

	cv::Mat descriptors(3, 1, CV_8U);
	descriptors.at<uchar>(0, 0) = 4;
	descriptors.at<uchar>(0, 1) = 10;
	descriptors.at<uchar>(0, 2) = 5;
	FileUtils::saveDescriptors("descriptors.bin", descriptors);

	std::vector<std::string> descriptorsFilenames;
	descriptorsFilenames.push_back("descriptors.bin");
	vlr::Mat data(descriptorsFilenames);

	vlr::IncrementalKMeans obj(data);

	EXPECT_TRUE(obj.getDim() == data.cols);

	EXPECT_TRUE(obj.getNumDatapoints() == data.rows);

	EXPECT_TRUE(obj.getDataset().rows == data.rows);
	EXPECT_TRUE(obj.getDataset().cols == data.cols);

	EXPECT_TRUE(obj.getCentroids().rows == obj.getNumClusters());
	EXPECT_TRUE(obj.getCentroids().cols == data.cols * 8);

	EXPECT_TRUE(obj.getClustersVariances().rows == obj.getNumClusters());
	EXPECT_TRUE(obj.getClustersVariances().cols == data.cols * 8);

	EXPECT_TRUE(obj.getClustersWeights().rows == 1);
	EXPECT_TRUE(obj.getClustersWeights().cols == obj.getNumClusters());

	EXPECT_TRUE(obj.getClustersSums().rows == obj.getNumClusters());
	EXPECT_TRUE(obj.getClustersSums().cols == data.cols * 8);

	EXPECT_TRUE(obj.getClustersCounts().rows == 1);
	EXPECT_TRUE(obj.getClustersCounts().cols == obj.getNumClusters());

	EXPECT_TRUE(obj.getMiu().rows == 1);
	EXPECT_TRUE(obj.getMiu().cols == data.cols * 8);

	EXPECT_TRUE(obj.getSigma().rows == 1);
	EXPECT_TRUE(obj.getSigma().cols == data.cols * 8);

	EXPECT_TRUE(obj.getMiu().at<double>(0, 0) == ((double) 0.0));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 1) == ((double) 0.0));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 2) == ((double) 0.0));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 3) == ((double) 0.0));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 4) == ((double) 1/3));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 5) == ((double) 2/3));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 6) == ((double) 1/3));
	EXPECT_TRUE(obj.getMiu().at<double>(0, 7) == ((double) 1/3));

	EXPECT_TRUE(obj.getSigma().at<double>(0, 0) == ((double) 0.0));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 1) == ((double) 0.0));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 2) == ((double) 0.0));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 3) == ((double) 0.0));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 4) == sqrt((double) 6/27));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 5) == sqrt((double) 6/27));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 6) == sqrt((double) 6/27));
	EXPECT_TRUE(obj.getSigma().at<double>(0, 7) == sqrt((double) 6/27));

	EXPECT_TRUE(obj.getOutliers().empty());

}

TEST(IncrementalKMeans, InitWithRealData) {

	std::vector<std::string> descriptorsFilenames;
	descriptorsFilenames.push_back("brief.bin");
	vlr::Mat data(descriptorsFilenames);
	vlr::IncrementalKMeans obj(data);

	EXPECT_TRUE(obj.getDim() == data.cols);

	EXPECT_TRUE(obj.getNumDatapoints() == data.rows);

	EXPECT_TRUE(obj.getDataset().rows == data.rows);
	EXPECT_TRUE(obj.getDataset().cols == data.cols);

	EXPECT_TRUE(obj.getCentroids().rows == obj.getNumClusters());
	EXPECT_TRUE(obj.getCentroids().cols == data.cols * 8);

	EXPECT_TRUE(obj.getClustersVariances().rows == obj.getNumClusters());
	EXPECT_TRUE(obj.getClustersVariances().cols == data.cols * 8);

	EXPECT_TRUE(obj.getClustersWeights().rows == 1);
	EXPECT_TRUE(obj.getClustersWeights().cols == obj.getNumClusters());

	EXPECT_TRUE(obj.getClustersSums().rows == obj.getNumClusters());
	EXPECT_TRUE(obj.getClustersSums().cols == data.cols * 8);

	EXPECT_TRUE(obj.getClustersCounts().rows == 1);
	EXPECT_TRUE(obj.getClustersCounts().cols == obj.getNumClusters());

	EXPECT_TRUE(obj.getMiu().rows == 1);
	EXPECT_TRUE(obj.getMiu().cols == data.cols * 8);

	EXPECT_TRUE(obj.getSigma().rows == 1);
	EXPECT_TRUE(obj.getSigma().cols == data.cols * 8);

	EXPECT_TRUE(obj.getOutliers().empty());

}

//	inline class IncrementalKMeansPublic: public vlr::IncrementalKMeans {
//	public:
//		IncrementalKMeansPublic(vlr::IncrementalKMeans obj) {
//		}
//	};

TEST(IncrementalKMeans, InitCentroids) {

	std::vector<std::string> descriptorsFilenames;
	descriptorsFilenames.push_back("brief.bin");
	vlr::Mat data(descriptorsFilenames);
	vlr::IncrementalKMeansParams params;
	params["num.clusters"] = 10;
	vlr::IncrementalKMeans obj(data, params);

	obj.initCentroids();

	cv::Mat min = (obj.getMiu() - obj.getSigma() / (obj.getDim() * 8));
	cv::Mat max = (obj.getMiu() + obj.getSigma() / (obj.getDim() * 8));

	for (int j = 0; j < obj.getNumClusters(); ++j) {
		for (int l = 0; l < obj.getCentroids().cols; l++) {
			EXPECT_TRUE(obj.getCentroids().at<double>(j, l) >= min.at<double>(0, l));
			EXPECT_TRUE(obj.getCentroids().at<double>(j, l) <= max.at<double>(0, l));
		}
	}

}

TEST(IncrementalKMeans, PreComputeDistances) {

	std::vector<std::string> descriptorsFilenames;
	descriptorsFilenames.push_back("brief.bin");
	vlr::Mat data(descriptorsFilenames);
	vlr::IncrementalKMeansParams params;
	params["num.clusters"] = 10;
	vlr::IncrementalKMeans obj(data, params);

	obj.initCentroids();
	obj.preComputeDistances();

	cv::Mat temp;
	for (int j = 0; j < obj.getNumClusters(); ++j) {
		cv::pow(-obj.getCentroids().row(j), 2, temp);
		EXPECT_TRUE(obj.getClusterDistancesToNullTransaction().at<double>(0, j) == cv::sum(temp).val[0]);
	}

}

TEST(IncrementalKMeans, FindNearestNeighbor) {

	std::vector<std::string> descriptorsFilenames;
	descriptorsFilenames.push_back("brief.bin");
	vlr::Mat data(descriptorsFilenames);
	vlr::IncrementalKMeansParams params;
	params["num.clusters"] = 10;
	vlr::IncrementalKMeans vocabTrainer(data, params);

	vocabTrainer.initCentroids();
	vocabTrainer.preComputeDistances();
	vocabTrainer.initClustersCounters();

	uchar byte = 0;
	cv::Mat transaction;
	int clusterIndex, sparseClusterIndex;
	double distanceToCluster, tempDistanceToCluster, sparseDistanceToCluster;
	for (int i = 0; i < vocabTrainer.getNumDatapoints(); ++i) {
		transaction = data.row(i);
		// Find nearest neighbor to ith transaction
		clusterIndex = 0;
		distanceToCluster = std::numeric_limits<double>::max();
		for (int j = 0; j < vocabTrainer.getNumClusters(); ++j) {
			tempDistanceToCluster = 0;
			for (int l = 0; l < vocabTrainer.getDim() * 8; l++) {
				if ((l % 8) == 0) {
					byte = *(transaction.col((int) l / 8).data);
				}
				int bit = ((int) ((byte >> (7 - (l % 8))) % 2));
				tempDistanceToCluster += pow(bit - vocabTrainer.getCentroids().at<double>(j, l), 2);
			}
			if (tempDistanceToCluster < distanceToCluster) {
				clusterIndex = j;
				distanceToCluster = tempDistanceToCluster;
			}
		}
		// Find nearest neighbor to ith transaction in a sparse manner
		vocabTrainer.findNearestNeighbor(transaction, sparseClusterIndex, sparseDistanceToCluster);
		EXPECT_TRUE(clusterIndex == sparseClusterIndex);
		EXPECT_TRUE(fabs(distanceToCluster - sparseDistanceToCluster) < 0.00000001);
	}

}

//TEST(IncrementalKMeans, SparseSum) {
//
//	std::vector<std::string> descriptorsFilenames;
//	descriptorsFilenames.push_back("brief.bin");
//	vlr::Mat data(descriptorsFilenames);
//	vlr::IncrementalKMeans obj(data);
//
////	obj.sparseSum();
//
//}
//
//TEST(IncrementalKMeans, SparseSubtraction) {
//
//	std::vector<std::string> descriptorsFilenames;
//	descriptorsFilenames.push_back("brief.bin");
//	vlr::Mat data(descriptorsFilenames);
//	vlr::IncrementalKMeans obj(data);
//
////	obj.sparseSubtraction();
//
//}
//
//TEST(IncrementalKMeans, InsertOutlier) {
//
//	std::vector<std::string> descriptorsFilenames;
//	descriptorsFilenames.push_back("brief.bin");
//	vlr::Mat data(descriptorsFilenames);
//	vlr::IncrementalKMeans obj(data);
//
////	obj.insertOutlier();
//
//}
//
//TEST(IncrementalKMeans, ComputeCentroids) {
//
//	std::vector<std::string> descriptorsFilenames;
//	descriptorsFilenames.push_back("brief.bin");
//	vlr::Mat data(descriptorsFilenames);
//	vlr::IncrementalKMeans obj(data);
//
////	obj.computeCentroids();
//
//}
//
//TEST(IncrementalKMeans, HandleEmptyClusters) {
//
//	std::vector<std::string> descriptorsFilenames;
//	descriptorsFilenames.push_back("brief.bin");
//	vlr::Mat data(descriptorsFilenames);
//	vlr::IncrementalKMeans obj(data);
//
////	obj.handleEmptyClusters();
//
//}
//
//TEST(IncrementalKMeans, Build) {
//
//	std::vector<std::string> descriptorsFilenames;
//	descriptorsFilenames.push_back("brief.bin");
//	vlr::Mat data(descriptorsFilenames);
//	vlr::IncrementalKMeans obj(data);
//
////	obj.build();
//
//}

} /* namespace vlr */
