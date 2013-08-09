/*
 * icp_map.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: vsu
 */

#include <icp_map.h>
#include <boost/filesystem.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>

keyframe::keyframe(const cv::Mat & rgb, const cv::Mat & depth,
		const Sophus::SE3f & position,
		std::vector<Eigen::Vector3f> & intrinsics_vector, int intrinsics_idx) :
		rgb(rgb), depth(depth), position(position), initial_position(position), intrinsics_vector(
				intrinsics_vector), intrinsics_idx(intrinsics_idx) {

	Eigen::Vector3f & intrinsics = intrinsics_vector[intrinsics_idx];

	int num_points = 0;
	centroid.setZero();

	for (int v = 0; v < depth.rows; v++) {
		for (int u = 0; u < depth.cols; u++) {
			if (depth.at<unsigned short>(v, u) != 0) {
				pcl::PointXYZ p;
				p.z = depth.at<unsigned short>(v, u) / 1000.0f;
				p.x = (u - intrinsics[1]) * p.z / intrinsics[0];
				p.y = (v - intrinsics[2]) * p.z / intrinsics[0];

				centroid += p.getVector3fMap();
				num_points++;

			}
		}
	}

	centroid /= num_points;

	cv::Mat tmp;
	cv::cvtColor(rgb, tmp, CV_RGB2GRAY);
	tmp.convertTo(intencity, CV_32F, 1 / 255.0);

}

Eigen::Vector3f keyframe::get_centroid() const {
	return position * centroid;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr keyframe::get_pointcloud() const {
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(
			new pcl::PointCloud<pcl::PointXYZ>);

	Eigen::Vector3f & intrinsics = intrinsics_vector[intrinsics_idx];

	for (int v = 0; v < depth.rows; v += 2) {
		for (int u = 0; u < depth.cols; u += 2) {
			if (depth.at<unsigned short>(v, u) != 0) {
				pcl::PointXYZ p;
				p.z = depth.at<unsigned short>(v, u) / 1000.0f;
				p.x = (u - intrinsics[1]) * p.z / intrinsics[0];
				p.y = (v - intrinsics[2]) * p.z / intrinsics[0];

				p.getVector3fMap() = position * p.getVector3fMap();

				cloud->push_back(p);

			}
		}
	}

	return cloud;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr keyframe::get_pointcloud(float min_height,
		float max_height) const {
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(
			new pcl::PointCloud<pcl::PointXYZ>);

	Eigen::Vector3f & intrinsics = intrinsics_vector[intrinsics_idx];

	for (int v = 0; v < depth.rows; v += 2) {
		for (int u = 0; u < depth.cols; u += 2) {
			if (depth.at<unsigned short>(v, u) != 0) {
				pcl::PointXYZ p;
				p.z = depth.at<unsigned short>(v, u) / 1000.0f;
				p.x = (u - intrinsics[1]) * p.z / intrinsics[0];
				p.y = (v - intrinsics[2]) * p.z / intrinsics[0];

				p.getVector3fMap() = position * p.getVector3fMap();

				if (p.z > min_height && p.z < max_height)
					cloud->push_back(p);

			}
		}
	}

	return cloud;
}

pcl::PointCloud<pcl::PointXYZRGB>::Ptr keyframe::get_colored_pointcloud() const {
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(
			new pcl::PointCloud<pcl::PointXYZRGB>);

	Eigen::Vector3f & intrinsics = intrinsics_vector[intrinsics_idx];

	for (int v = 0; v < depth.rows; v += 2) {
		for (int u = 0; u < depth.cols; u += 2) {
			if (depth.at<unsigned short>(v, u) != 0) {

				cv::Vec3b color = rgb.at<cv::Vec3b>(v, u);

				pcl::PointXYZRGB p;
				p.z = depth.at<unsigned short>(v, u) / 1000.0f;
				p.x = (u - intrinsics[1]) * p.z / intrinsics[0];
				p.y = (v - intrinsics[2]) * p.z / intrinsics[0];

				p.b = color[0];
				p.g = color[1];
				p.r = color[2];

				p.getVector3fMap() = position * p.getVector3fMap();

				cloud->push_back(p);

			}
		}
	}

	return cloud;
}

pcl::PointCloud<pcl::PointXYZRGB>::Ptr keyframe::get_colored_pointcloud(
		float min_height, float max_height) const {
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(
			new pcl::PointCloud<pcl::PointXYZRGB>);

	Eigen::Vector3f & intrinsics = intrinsics_vector[intrinsics_idx];

	for (int v = 0; v < depth.rows; v += 2) {
		for (int u = 0; u < depth.cols; u += 2) {
			if (depth.at<unsigned short>(v, u) != 0) {

				cv::Vec3b color = rgb.at<cv::Vec3b>(v, u);

				pcl::PointXYZRGB p;
				p.z = depth.at<unsigned short>(v, u) / 1000.0f;
				p.x = (u - intrinsics[1]) * p.z / intrinsics[0];
				p.y = (v - intrinsics[2]) * p.z / intrinsics[0];

				p.b = color[0];
				p.g = color[1];
				p.r = color[2];

				p.getVector3fMap() = position * p.getVector3fMap();

				if (p.z > min_height && p.z < max_height)
					cloud->push_back(p);

			}
		}
	}

	return cloud;
}

Sophus::SE3f & keyframe::get_position() {
	return position;
}

Sophus::SE3f & keyframe::get_initial_position() {
	return initial_position;
}

cv::Mat keyframe::get_subsampled_intencity(int level) {

	if (level > 0) {
		int size_reduction = 1 << level;
		cv::Mat res(rgb.rows / size_reduction, rgb.cols / size_reduction,
				intencity.type());

		for (int v = 0; v < res.rows; v++) {
			for (int u = 0; u < res.cols; u++) {

				float value = 0;

				for (int i = 0; i < size_reduction; i++) {
					for (int j = 0; j < size_reduction; j++) {
						value += intencity.at<float>(size_reduction * v + i,
								size_reduction * u + j);
					}
				}

				value /= size_reduction * size_reduction;
				res.at<float>(v, u) = value;

			}
		}

		//std::cerr << "Res size" << res.rows << " " << res.cols << std::endl;
		return res;

	} else {
		return intencity;
	}

}
Eigen::Vector3f keyframe::get_subsampled_intrinsics(int level) {

	Eigen::Vector3f & intrinsics = intrinsics_vector[intrinsics_idx];
	int size_reduction = 1 << level;
	return intrinsics / size_reduction;
}

int keyframe::get_intrinsics_idx() {
	return intrinsics_idx;
}

reduce_jacobian_icp::reduce_jacobian_icp(
		tbb::concurrent_vector<keyframe::Ptr> & frames, int size) :
		size(size), frames(frames) {

	JtJ.setZero(size * 6, size * 6);
	Jte.setZero(size * 6);

}

reduce_jacobian_icp::reduce_jacobian_icp(reduce_jacobian_icp& rb, tbb::split) :
		size(rb.size), frames(rb.frames) {
	JtJ.setZero(size * 6, size * 6);
	Jte.setZero(size * 6);
}

void reduce_jacobian_icp::operator()(
		const tbb::blocked_range<
				tbb::concurrent_vector<std::pair<int, int> >::iterator>& r) {
	for (tbb::concurrent_vector<std::pair<int, int> >::iterator it = r.begin();
			it != r.end(); it++) {
		int i = it->first;
		int j = it->second;

		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_i, cloud_j;

		cloud_i = frames[i]->get_pointcloud();
		cloud_j = frames[j]->get_pointcloud();

		pcl::Correspondences cor;
		ce.setInputCloud(cloud_j);
		ce.setInputTarget(cloud_i);
		ce.determineCorrespondences(cor, 0.5);

		cr.getRemainingCorrespondences(cor, cor);

		for (size_t k = 0; k < cor.size(); k++) {
			if (cor[k].index_match >= 0) {
				pcl::PointXYZ & pi = cloud_i->at(cor[k].index_match);
				pcl::PointXYZ & pj = cloud_j->at(cor[k].index_query);

				Eigen::Vector3f error = pi.getVector3fMap()
						- pj.getVector3fMap();
				Eigen::Matrix<float, 3, 6> Ji, Jj;
				Ji.block<3, 3>(0, 0) = Eigen::Matrix3f::Identity();
				Jj.block<3, 3>(0, 0) = -Eigen::Matrix3f::Identity();

				Ji.block<3, 3>(0, 3) << 0, pi.z, -pi.y, -pi.z, 0, pi.x, pi.y, -pi.x, 0;
				Jj.block<3, 3>(0, 3) << 0, -pj.z, pj.y, pj.z, 0, -pj.x, -pj.y, pj.x, 0;

				JtJ.block<6, 6>(i * 6, i * 6) += Ji.transpose() * Ji;
				JtJ.block<6, 6>(j * 6, j * 6) += Jj.transpose() * Jj;
				JtJ.block<6, 6>(i * 6, j * 6) += Ji.transpose() * Jj;
				JtJ.block<6, 6>(j * 6, i * 6) += Jj.transpose() * Ji;

				Jte.segment<6>(i * 6) += Ji.transpose() * error;
				Jte.segment<6>(j * 6) += Jj.transpose() * error;

			}

		}
	}
}

void reduce_jacobian_icp::join(reduce_jacobian_icp& rb) {
	JtJ += rb.JtJ;
	Jte += rb.Jte;
}

reduce_jacobian_rgb::reduce_jacobian_rgb(
		tbb::concurrent_vector<keyframe::Ptr> & frames,
		std::vector<Eigen::Vector3f> & intrinsics_vector, int size,
		int intrinsics_size, int subsample_level) :
		size(size), intrinsics_size(intrinsics_size), subsample_level(
				subsample_level), frames(frames), intrinsics_vector(
				intrinsics_vector) {

	JtJ.setZero(size * 3 + 3 * intrinsics_size, size * 3 + 3 * intrinsics_size);
	Jte.setZero(size * 3 + 3 * intrinsics_size);

}

reduce_jacobian_rgb::reduce_jacobian_rgb(reduce_jacobian_rgb& rb, tbb::split) :
		size(rb.size), intrinsics_size(rb.intrinsics_size), subsample_level(
				rb.subsample_level), frames(rb.frames), intrinsics_vector(
				rb.intrinsics_vector) {
	JtJ.setZero(size * 3 + 3 * intrinsics_size, size * 3 + 3 * intrinsics_size);
	Jte.setZero(size * 3 + 3 * intrinsics_size);
}

void reduce_jacobian_rgb::compute_frame_jacobian(const Eigen::Vector3f & i,
		const Eigen::Matrix3f & Rwi, const Eigen::Matrix3f & Rwj,
		Eigen::Matrix<float, 9, 3> & Ji, Eigen::Matrix<float, 9, 3> & Jj,
		Eigen::Matrix<float, 9, 3> & Jk) {

	Ji.coeffRef(0, 0) = (-Rwj(1, 0) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			+ Rwj(2, 0) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))) / i(0);
	Ji.coeffRef(0, 1) = (Rwj(0, 0) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			- Rwj(2, 0) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Ji.coeffRef(0, 2) = (-Rwj(0, 0) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
			+ Rwj(1, 0) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Ji.coeffRef(1, 0) = (-Rwj(1, 1) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			+ Rwj(2, 1) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))) / i(0);
	Ji.coeffRef(1, 1) = (Rwj(0, 1) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			- Rwj(2, 1) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Ji.coeffRef(1, 2) = (-Rwj(0, 1) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
			+ Rwj(1, 1) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Ji.coeffRef(2, 0) = -((Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			- (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
					* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0)))
			/ i(0);
	Ji.coeffRef(2, 1) = (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))
			* (Rwj(2, 0) * i(1) / i(0) + Rwj(2, 1) * i(2) / i(0) - Rwj(2, 2))
			- (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
					* (Rwj(0, 0) * i(1) / i(0) + Rwj(0, 1) * i(2) / i(0)
							- Rwj(0, 2));
	Ji.coeffRef(2, 2) = -((Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))
			* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0))
			- (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);
	Ji.coeffRef(3, 0) = (-Rwj(1, 0) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			+ Rwj(2, 0) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))) / i(0);
	Ji.coeffRef(3, 1) = (Rwj(0, 0) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			- Rwj(2, 0) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Ji.coeffRef(3, 2) = (-Rwj(0, 0) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
			+ Rwj(1, 0) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Ji.coeffRef(4, 0) = (-Rwj(1, 1) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			+ Rwj(2, 1) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))) / i(0);
	Ji.coeffRef(4, 1) = (Rwj(0, 1) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			- Rwj(2, 1) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Ji.coeffRef(4, 2) = (-Rwj(0, 1) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
			+ Rwj(1, 1) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Ji.coeffRef(5, 0) = -((Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			- (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
					* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0)))
			/ i(0);
	Ji.coeffRef(5, 1) = (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))
			* (Rwj(2, 0) * i(1) / i(0) + Rwj(2, 1) * i(2) / i(0) - Rwj(2, 2))
			- (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
					* (Rwj(0, 0) * i(1) / i(0) + Rwj(0, 1) * i(2) / i(0)
							- Rwj(0, 2));
	Ji.coeffRef(5, 2) = -((Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))
			* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0))
			- (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);
	Ji.coeffRef(6, 0) = (Rwi(1, 2) * Rwj(2, 0) - Rwi(2, 2) * Rwj(1, 0)) / i(0);
	Ji.coeffRef(6, 1) = (-Rwi(0, 2) * Rwj(2, 0) + Rwi(2, 2) * Rwj(0, 0)) / i(0);
	Ji.coeffRef(6, 2) = (Rwi(0, 2) * Rwj(1, 0) - Rwi(1, 2) * Rwj(0, 0)) / i(0);
	Ji.coeffRef(7, 0) = (Rwi(1, 2) * Rwj(2, 1) - Rwi(2, 2) * Rwj(1, 1)) / i(0);
	Ji.coeffRef(7, 1) = (-Rwi(0, 2) * Rwj(2, 1) + Rwi(2, 2) * Rwj(0, 1)) / i(0);
	Ji.coeffRef(7, 2) = (Rwi(0, 2) * Rwj(1, 1) - Rwi(1, 2) * Rwj(0, 1)) / i(0);
	Ji.coeffRef(8, 0) = (-Rwi(1, 2)
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			+ Rwi(2, 2)
					* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0)))
			/ i(0);
	Ji.coeffRef(8, 1) = (Rwi(0, 2)
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			- Rwi(2, 2)
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);
	Ji.coeffRef(8, 2) = (-Rwi(0, 2)
			* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0))
			+ Rwi(1, 2)
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);

	Jj.coeffRef(0, 0) = (Rwj(1, 0) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			- Rwj(2, 0) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))) / i(0);
	Jj.coeffRef(0, 1) = (-Rwj(0, 0) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			+ Rwj(2, 0) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Jj.coeffRef(0, 2) = (Rwj(0, 0) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
			- Rwj(1, 0) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Jj.coeffRef(1, 0) = (Rwj(1, 1) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			- Rwj(2, 1) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))) / i(0);
	Jj.coeffRef(1, 1) = (-Rwj(0, 1) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
			+ Rwj(2, 1) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Jj.coeffRef(1, 2) = (Rwj(0, 1) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
			- Rwj(1, 1) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))) / i(0);
	Jj.coeffRef(2, 0) = (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
			* (Rwj(2, 0) * i(1) / i(0) + Rwj(2, 1) * i(2) / i(0) - Rwj(2, 2))
			- (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
					* (Rwj(1, 0) * i(1) / i(0) + Rwj(1, 1) * i(2) / i(0)
							- Rwj(1, 2));
	Jj.coeffRef(2, 1) = -((Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			- (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);
	Jj.coeffRef(2, 2) = (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))
			* (Rwj(1, 0) * i(1) / i(0) + Rwj(1, 1) * i(2) / i(0) - Rwj(1, 2))
			- (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
					* (Rwj(0, 0) * i(1) / i(0) + Rwj(0, 1) * i(2) / i(0)
							- Rwj(0, 2));
	Jj.coeffRef(3, 0) = (Rwj(1, 0) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			- Rwj(2, 0) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))) / i(0);
	Jj.coeffRef(3, 1) = (-Rwj(0, 0) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			+ Rwj(2, 0) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Jj.coeffRef(3, 2) = (Rwj(0, 0) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
			- Rwj(1, 0) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Jj.coeffRef(4, 0) = (Rwj(1, 1) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			- Rwj(2, 1) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))) / i(0);
	Jj.coeffRef(4, 1) = (-Rwj(0, 1) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
			+ Rwj(2, 1) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Jj.coeffRef(4, 2) = (Rwj(0, 1) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
			- Rwj(1, 1) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))) / i(0);
	Jj.coeffRef(5, 0) = (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
			* (Rwj(2, 0) * i(1) / i(0) + Rwj(2, 1) * i(2) / i(0) - Rwj(2, 2))
			- (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
					* (Rwj(1, 0) * i(1) / i(0) + Rwj(1, 1) * i(2) / i(0)
							- Rwj(1, 2));
	Jj.coeffRef(5, 1) = -((Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			- (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);
	Jj.coeffRef(5, 2) = (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))
			* (Rwj(1, 0) * i(1) / i(0) + Rwj(1, 1) * i(2) / i(0) - Rwj(1, 2))
			- (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
					* (Rwj(0, 0) * i(1) / i(0) + Rwj(0, 1) * i(2) / i(0)
							- Rwj(0, 2));
	Jj.coeffRef(6, 0) = (-Rwi(1, 2) * Rwj(2, 0) + Rwi(2, 2) * Rwj(1, 0)) / i(0);
	Jj.coeffRef(6, 1) = (Rwi(0, 2) * Rwj(2, 0) - Rwi(2, 2) * Rwj(0, 0)) / i(0);
	Jj.coeffRef(6, 2) = (-Rwi(0, 2) * Rwj(1, 0) + Rwi(1, 2) * Rwj(0, 0)) / i(0);
	Jj.coeffRef(7, 0) = (-Rwi(1, 2) * Rwj(2, 1) + Rwi(2, 2) * Rwj(1, 1)) / i(0);
	Jj.coeffRef(7, 1) = (Rwi(0, 2) * Rwj(2, 1) - Rwi(2, 2) * Rwj(0, 1)) / i(0);
	Jj.coeffRef(7, 2) = (-Rwi(0, 2) * Rwj(1, 1) + Rwi(1, 2) * Rwj(0, 1)) / i(0);
	Jj.coeffRef(8, 0) = (Rwi(1, 2)
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			- Rwi(2, 2)
					* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0)))
			/ i(0);
	Jj.coeffRef(8, 1) = (-Rwi(0, 2)
			* (Rwj(2, 0) * i(1) + Rwj(2, 1) * i(2) - Rwj(2, 2) * i(0))
			+ Rwi(2, 2)
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);
	Jj.coeffRef(8, 2) = (Rwi(0, 2)
			* (Rwj(1, 0) * i(1) + Rwj(1, 1) * i(2) - Rwj(1, 2) * i(0))
			- Rwi(1, 2)
					* (Rwj(0, 0) * i(1) + Rwj(0, 1) * i(2) - Rwj(0, 2) * i(0)))
			/ i(0);

	Jk.coeffRef(0, 0) = i(1)
			* (-Rwi(0, 2) * Rwj(0, 0) - Rwi(1, 2) * Rwj(1, 0)
					- Rwi(2, 2) * Rwj(2, 0)) / i(0);
	Jk.coeffRef(0, 1) = i(1)
			* (Rwi(0, 2) * Rwj(0, 0) + Rwi(1, 2) * Rwj(1, 0)
					+ Rwi(2, 2) * Rwj(2, 0)) / i(0);
	Jk.coeffRef(0, 2) = 0;
	Jk.coeffRef(1, 0) = i(1)
			* (-Rwi(0, 2) * Rwj(0, 1) - Rwi(1, 2) * Rwj(1, 1)
					- Rwi(2, 2) * Rwj(2, 1)) / i(0);
	Jk.coeffRef(1, 1) = i(1)
			* (Rwi(0, 2) * Rwj(0, 1) + Rwi(1, 2) * Rwj(1, 1)
					+ Rwi(2, 2) * Rwj(2, 1)) / i(0);
	Jk.coeffRef(1, 2) = 0;
	Jk.coeffRef(2, 0) = (Rwi(0, 0) * Rwj(0, 2) * i(0) * i(0)
			+ Rwi(0, 2) * Rwj(0, 0) * i(1) * i(1)
			+ Rwi(0, 2) * Rwj(0, 1) * i(1) * i(2)
			+ Rwi(1, 0) * Rwj(1, 2) * i(0) * i(0)
			+ Rwi(1, 2) * Rwj(1, 0) * i(1) * i(1)
			+ Rwi(1, 2) * Rwj(1, 1) * i(1) * i(2)
			+ Rwi(2, 0) * Rwj(2, 2) * i(0) * i(0)
			+ Rwi(2, 2) * Rwj(2, 0) * i(1) * i(1)
			+ Rwi(2, 2) * Rwj(2, 1) * i(1) * i(2)) / i(0);
	Jk.coeffRef(2, 1) = i(1)
			* (-Rwi(0, 0) * Rwj(0, 0) * i(0) - 2 * Rwi(0, 2) * Rwj(0, 0) * i(1)
					- Rwi(0, 2) * Rwj(0, 1) * i(2)
					+ Rwi(0, 2) * Rwj(0, 2) * i(0)
					- Rwi(1, 0) * Rwj(1, 0) * i(0)
					- 2 * Rwi(1, 2) * Rwj(1, 0) * i(1)
					- Rwi(1, 2) * Rwj(1, 1) * i(2)
					+ Rwi(1, 2) * Rwj(1, 2) * i(0)
					- Rwi(2, 0) * Rwj(2, 0) * i(0)
					- 2 * Rwi(2, 2) * Rwj(2, 0) * i(1)
					- Rwi(2, 2) * Rwj(2, 1) * i(2)
					+ Rwi(2, 2) * Rwj(2, 2) * i(0)) / i(0);
	Jk.coeffRef(2, 2) = -i(2)
			* (Rwj(0, 1) * (Rwi(0, 0) * i(0) + Rwi(0, 2) * i(1))
					+ Rwj(1, 1) * (Rwi(1, 0) * i(0) + Rwi(1, 2) * i(1))
					+ Rwj(2, 1) * (Rwi(2, 0) * i(0) + Rwi(2, 2) * i(1))) / i(0);
	Jk.coeffRef(3, 0) = i(2)
			* (-Rwi(0, 2) * Rwj(0, 0) - Rwi(1, 2) * Rwj(1, 0)
					- Rwi(2, 2) * Rwj(2, 0)) / i(0);
	Jk.coeffRef(3, 1) = 0;
	Jk.coeffRef(3, 2) = i(2)
			* (Rwi(0, 2) * Rwj(0, 0) + Rwi(1, 2) * Rwj(1, 0)
					+ Rwi(2, 2) * Rwj(2, 0)) / i(0);
	Jk.coeffRef(4, 0) = i(2)
			* (-Rwi(0, 2) * Rwj(0, 1) - Rwi(1, 2) * Rwj(1, 1)
					- Rwi(2, 2) * Rwj(2, 1)) / i(0);
	Jk.coeffRef(4, 1) = 0;
	Jk.coeffRef(4, 2) = i(2)
			* (Rwi(0, 2) * Rwj(0, 1) + Rwi(1, 2) * Rwj(1, 1)
					+ Rwi(2, 2) * Rwj(2, 1)) / i(0);
	Jk.coeffRef(5, 0) = (Rwi(0, 1) * Rwj(0, 2) * i(0) * i(0)
			+ Rwi(0, 2) * Rwj(0, 0) * i(1) * i(2)
			+ Rwi(0, 2) * Rwj(0, 1) * i(2) * i(2)
			+ Rwi(1, 1) * Rwj(1, 2) * i(0) * i(0)
			+ Rwi(1, 2) * Rwj(1, 0) * i(1) * i(2)
			+ Rwi(1, 2) * Rwj(1, 1) * i(2) * i(2)
			+ Rwi(2, 1) * Rwj(2, 2) * i(0) * i(0)
			+ Rwi(2, 2) * Rwj(2, 0) * i(1) * i(2)
			+ Rwi(2, 2) * Rwj(2, 1) * i(2) * i(2)) / i(0);
	Jk.coeffRef(5, 1) = -i(1)
			* (Rwj(0, 0) * (Rwi(0, 1) * i(0) + Rwi(0, 2) * i(2))
					+ Rwj(1, 0) * (Rwi(1, 1) * i(0) + Rwi(1, 2) * i(2))
					+ Rwj(2, 0) * (Rwi(2, 1) * i(0) + Rwi(2, 2) * i(2))) / i(0);
	Jk.coeffRef(5, 2) = i(2)
			* (-Rwi(0, 1) * Rwj(0, 1) * i(0) - Rwi(0, 2) * Rwj(0, 0) * i(1)
					- 2 * Rwi(0, 2) * Rwj(0, 1) * i(2)
					+ Rwi(0, 2) * Rwj(0, 2) * i(0)
					- Rwi(1, 1) * Rwj(1, 1) * i(0)
					- Rwi(1, 2) * Rwj(1, 0) * i(1)
					- 2 * Rwi(1, 2) * Rwj(1, 1) * i(2)
					+ Rwi(1, 2) * Rwj(1, 2) * i(0)
					- Rwi(2, 1) * Rwj(2, 1) * i(0)
					- Rwi(2, 2) * Rwj(2, 0) * i(1)
					- 2 * Rwi(2, 2) * Rwj(2, 1) * i(2)
					+ Rwi(2, 2) * Rwj(2, 2) * i(0)) / i(0);
	Jk.coeffRef(6, 0) = -(Rwi(0, 2) * Rwj(0, 0) + Rwi(1, 2) * Rwj(1, 0)
			+ Rwi(2, 2) * Rwj(2, 0)) / i(0);
	Jk.coeffRef(6, 1) = 0;
	Jk.coeffRef(6, 2) = 0;
	Jk.coeffRef(7, 0) = -(Rwi(0, 2) * Rwj(0, 1) + Rwi(1, 2) * Rwj(1, 1)
			+ Rwi(2, 2) * Rwj(2, 1)) / i(0);
	Jk.coeffRef(7, 1) = 0;
	Jk.coeffRef(7, 2) = 0;
	Jk.coeffRef(8, 0) = (i(1)
			* (Rwi(0, 2) * Rwj(0, 0) + Rwi(1, 2) * Rwj(1, 0)
					+ Rwi(2, 2) * Rwj(2, 0))
			+ i(2)
					* (Rwi(0, 2) * Rwj(0, 1) + Rwi(1, 2) * Rwj(1, 1)
							+ Rwi(2, 2) * Rwj(2, 1))) / i(0);
	Jk.coeffRef(8, 1) = i(1)
			* (-Rwi(0, 2) * Rwj(0, 0) - Rwi(1, 2) * Rwj(1, 0)
					- Rwi(2, 2) * Rwj(2, 0)) / i(0);
	Jk.coeffRef(8, 2) = i(2)
			* (-Rwi(0, 2) * Rwj(0, 1) - Rwi(1, 2) * Rwj(1, 1)
					- Rwi(2, 2) * Rwj(2, 1)) / i(0);

}

void reduce_jacobian_rgb::operator()(
		const tbb::blocked_range<
				tbb::concurrent_vector<std::pair<int, int> >::iterator>& r) {
	for (tbb::concurrent_vector<std::pair<int, int> >::iterator it = r.begin();
			it != r.end(); it++) {
		int i = it->first;
		int j = it->second;

		Eigen::Vector3f intrinsics = frames[i]->get_subsampled_intrinsics(
				subsample_level);
		cv::Mat intensity_i = frames[i]->get_subsampled_intencity(
				subsample_level);
		cv::Mat intensity_j = frames[j]->get_subsampled_intencity(
				subsample_level);

		Eigen::Quaternionf Qij =
				frames[i]->get_position().unit_quaternion().inverse()
						* frames[j]->get_position().unit_quaternion();

		Eigen::Matrix3f K;
		K << intrinsics[0], 0, intrinsics[1], 0, intrinsics[0], intrinsics[2], 0, 0, 1;

		Eigen::Matrix3f H = K * Qij.matrix() * K.inverse();
		cv::Mat cvH(3, 3, CV_32F, H.data());

		//std::cerr << "Intrinsics" << std::endl << intrinsics << std::endl << "H"
		//		<< std::endl << H << std::endl << "cvH" << std::endl << cvH
		//		<< std::endl;

		cv::Mat intensity_j_warped, intensity_i_dx, intensity_i_dy;
		intensity_j_warped = cv::Mat::zeros(intensity_j.size(),
				intensity_j.type());
		cv::warpPerspective(intensity_j, intensity_j_warped, cvH.t(),
				intensity_j.size());

		//cv::imshow("intensity_i", intensity_i);
		//cv::imshow("intensity_j", intensity_j);
		//cv::imshow("intensity_j_warped", intensity_j_warped);
		//cv::waitKey();

		cv::Sobel(intensity_i, intensity_i_dx, CV_32F, 1, 0);
		cv::Sobel(intensity_i, intensity_i_dy, CV_32F, 0, 1);

		cv::Mat error = intensity_i - intensity_j_warped;

		int ki = frames[i]->get_intrinsics_idx();
		int kj = frames[j]->get_intrinsics_idx();

		if (ki == kj) {

			Eigen::Matrix<float, 9, 3> Jwi, Jwj, Jwk;

			compute_frame_jacobian(intrinsics,
					frames[i]->get_position().unit_quaternion().matrix(),
					frames[j]->get_position().unit_quaternion().matrix(), Jwi,
					Jwj, Jwk);

			for (int v = 0; v < intensity_i.rows; v++) {
				for (int u = 0; u < intensity_i.cols; u++) {
					if (intensity_j_warped.at<float>(v, u) != 0) {

						float e = error.at<float>(v, u);

						float dx = intensity_i_dx.at<float>(v, u);
						float dy = intensity_i_dy.at<float>(v, u);
						float udx = dx * u;
						float vdx = dx * v;
						float udy = dy * u;
						float vdy = dy * v;

						float mudxmvdy = -udx - vdy;

						Eigen::Matrix<float, 1, 9> Jp;
						Jp << udx, vdx, dx, udy, vdy, dy, u * mudxmvdy, v
								* mudxmvdy, mudxmvdy;

						Eigen::Matrix<float, 1, 3> Ji, Jj, Jk;
						Ji = Jp * Jwi;
						Jj = Jp * Jwj;
						Jk = Jp * Jwk;

						//
						JtJ.block<3, 3>(i * 3, i * 3) += Ji.transpose() * Ji;
						JtJ.block<3, 3>(j * 3, j * 3) += Jj.transpose() * Jj;
						JtJ.block<3, 3>(size * 3 + ki, size * 3 + ki) +=
								Jk.transpose() * Jk;
						// i and j
						JtJ.block<3, 3>(i * 3, j * 3) += Ji.transpose() * Jj;
						JtJ.block<3, 3>(j * 3, i * 3) += Jj.transpose() * Ji;

						// i and k
						JtJ.block<3, 3>(i * 3, size * 3 + ki) += Ji.transpose()
								* Jk;
						JtJ.block<3, 3>(size * 3 + ki, i * 3) += Jk.transpose()
								* Ji;

						// j and k
						JtJ.block<3, 3>(size * 3 + ki, j * 3) += Jk.transpose()
								* Jj;
						JtJ.block<3, 3>(j * 3, size * 3 + ki) += Jj.transpose()
								* Jk;

						// errors
						Jte.segment<3>(i * 3) += Ji.transpose() * e;
						Jte.segment<3>(j * 3) += Jj.transpose() * e;
						Jte.segment<3>(size * 3 + ki) += Jk.transpose() * e;

					}
				}
			}
		}

	}
}

void reduce_jacobian_rgb::join(reduce_jacobian_rgb& rb) {
	JtJ += rb.JtJ;
	Jte += rb.Jte;
}

icp_map::icp_map()
// : optimization_loop_thread(boost::bind(&icp_map::optimization_loop, this)) {
{
	Eigen::Vector3f intrinsics;
	intrinsics << 525.0, 319.5, 239.5;
	intrinsics /= 2;
	intrinsics_vector.push_back(intrinsics);
}

icp_map::keyframe_reference icp_map::add_frame(const cv::Mat rgb,
		const cv::Mat depth, const Sophus::SE3f & transform) {
	keyframe::Ptr k(new keyframe(rgb, depth, transform, intrinsics_vector, 0));
	return frames.push_back(k);
}

void icp_map::optimize() {

	int size = frames.size();

	tbb::concurrent_vector<std::pair<int, int> > overlaping_keyframes;

	for (int i = 0; i < size; i++) {

		for (int j = 0; j < i; j++) {

			float centroid_distance = (frames[i]->get_centroid()
					- frames[j]->get_centroid()).squaredNorm();

			Eigen::Quaternionf diff_quat =
					frames[i]->get_position().unit_quaternion()
							* frames[j]->get_position().unit_quaternion().inverse();

			float angle = 2 * std::acos(std::abs(diff_quat.w()));

			if (angle < M_PI / 6) {
				overlaping_keyframes.push_back(std::make_pair(i, j));
				//std::cerr << i << " and " << j
				//		<< " are connected with angle distance " << angle
				//		<< std::endl;
			}
		}
	}

	reduce_jacobian_icp rj(frames, size);

	tbb::parallel_reduce(
			tbb::blocked_range<
					tbb::concurrent_vector<std::pair<int, int> >::iterator>(
					overlaping_keyframes.begin(), overlaping_keyframes.end()),
			rj);

	Eigen::VectorXf update =
			-rj.JtJ.block(6, 6, (size - 1) * 6, (size - 1) * 6).ldlt().solve(
					rj.Jte.segment(6, (size - 1) * 6));

	std::cerr << "Max update " << update.maxCoeff() << " " << update.minCoeff()
			<< std::endl;

	position_modification_mutex.lock();
	for (int i = 0; i < size - 1; i++) {

		frames[i + 1]->get_position() = Sophus::SE3f::exp(
				update.segment<6>(i * 6)) * frames[i + 1]->get_position();

	}
	position_modification_mutex.unlock();

}

void icp_map::optimize_rgb(int level) {

	int size = frames.size();
	int intrinsics_size = intrinsics_vector.size();

	tbb::concurrent_vector<std::pair<int, int> > overlaping_keyframes;

	for (int i = 0; i < size; i++) {

		for (int j = 0; j < size; j++) {

			if (i != j) {

				float centroid_distance = (frames[i]->get_centroid()
						- frames[j]->get_centroid()).squaredNorm();

				Eigen::Quaternionf diff_quat =
						frames[i]->get_position().unit_quaternion()
								* frames[j]->get_position().unit_quaternion().inverse();

				float angle = 2 * std::acos(std::abs(diff_quat.w()));

				if (angle < M_PI / 6) {
					overlaping_keyframes.push_back(std::make_pair(i, j));
					//std::cerr << i << " and " << j
					//		<< " are connected with angle distance " << angle
					//		<< std::endl;
				}
			}
		}
	}

	reduce_jacobian_rgb rj(frames, intrinsics_vector, size, intrinsics_size,
			level);

	tbb::parallel_reduce(
			tbb::blocked_range<
					tbb::concurrent_vector<std::pair<int, int> >::iterator>(
					overlaping_keyframes.begin(), overlaping_keyframes.end()),
			rj);

	/*
	 rj(
	 tbb::blocked_range<
	 tbb::concurrent_vector<std::pair<int, int> >::iterator>(
	 overlaping_keyframes.begin(), overlaping_keyframes.end()));
	 */

	Eigen::VectorXf update =
			-rj.JtJ.block(3, 3, (size - 1) * 3, (size - 1) * 3).ldlt().solve(
					rj.Jte.segment(3, (size - 1) * 3));

	std::cerr << "Max update " << update.maxCoeff() << " " << update.minCoeff()
			<< std::endl;

	position_modification_mutex.lock();
	for (int i = 0; i < size - 1; i++) {

		Sophus::SO3f current_rotation = frames[i + 1]->get_position().so3();
		current_rotation = Sophus::SO3f::exp(update.segment<3>(i * 3))
				* current_rotation;

		frames[i + 1]->get_position().so3() = current_rotation;

	}

	position_modification_mutex.unlock();

	for (int i = 0; i < intrinsics_size; i++) {
		std::cerr << "Intrinsics" << std::endl << intrinsics_vector[i]
				<< std::endl;
	}

}

void icp_map::optimize_rgb_with_intrinsics(int level) {

	int size = frames.size();
	int intrinsics_size = intrinsics_vector.size();

	tbb::concurrent_vector<std::pair<int, int> > overlaping_keyframes;

	for (int i = 0; i < size; i++) {

		for (int j = 0; j < size; j++) {

			if (i != j) {

				float centroid_distance = (frames[i]->get_centroid()
						- frames[j]->get_centroid()).squaredNorm();

				Eigen::Quaternionf diff_quat =
						frames[i]->get_position().unit_quaternion()
								* frames[j]->get_position().unit_quaternion().inverse();

				float angle = 2 * std::acos(std::abs(diff_quat.w()));

				if (angle < M_PI / 6) {
					overlaping_keyframes.push_back(std::make_pair(i, j));
					//std::cerr << i << " and " << j
					//		<< " are connected with angle distance " << angle
					//		<< std::endl;
				}
			}
		}
	}

	reduce_jacobian_rgb rj(frames, intrinsics_vector, size, intrinsics_size,
			level);

	tbb::parallel_reduce(
			tbb::blocked_range<
					tbb::concurrent_vector<std::pair<int, int> >::iterator>(
					overlaping_keyframes.begin(), overlaping_keyframes.end()),
			rj);

	/*
	 rj(
	 tbb::blocked_range<
	 tbb::concurrent_vector<std::pair<int, int> >::iterator>(
	 overlaping_keyframes.begin(), overlaping_keyframes.end()));

	 */

	Eigen::VectorXf update = -rj.JtJ.block(3, 3,
			(size - 1) * 3 + intrinsics_size * 3,
			(size - 1) * 3 + intrinsics_size * 3).ldlt().solve(
			rj.Jte.segment(3, (size - 1) * 3 + intrinsics_size * 3));

	std::cerr << "Max update " << update.maxCoeff() << " " << update.minCoeff()
			<< std::endl;

	position_modification_mutex.lock();
	for (int i = 0; i < size - 1; i++) {

		Sophus::SO3f current_rotation = frames[i + 1]->get_position().so3();
		current_rotation = Sophus::SO3f::exp(update.segment<3>(i * 3))
				* current_rotation;

		frames[i + 1]->get_position().so3() = current_rotation;

	}

	for (int i = 0; i < intrinsics_size; i++) {
		intrinsics_vector[i] =
				update.segment<3>((size - 1) * 3 + i * 3).array().exp()
						* intrinsics_vector[i].array();
	}

	position_modification_mutex.unlock();

	for (int i = 0; i < intrinsics_size; i++) {
		std::cerr << "Intrinsics" << std::endl << intrinsics_vector[i]
				<< std::endl;
	}

}

pcl::PointCloud<pcl::PointXYZRGB>::Ptr icp_map::get_map_pointcloud() {
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr res(
			new pcl::PointCloud<pcl::PointXYZRGB>);

	for (size_t i = 0; i < frames.size(); i++) {

		pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud =
				frames[i]->get_colored_pointcloud();

		*res += *cloud;

	}

	return res;

}

cv::Mat icp_map::get_panorama_image() {
	cv::Mat res = cv::Mat::zeros(800, 1600, CV_32F);
	cv::Mat w = cv::Mat::zeros(res.size(), res.type());

	cv::Mat map_x(res.size(), res.type()), map_y(res.size(), res.type()),
			img_projected(res.size(), res.type());

	float cx = res.cols / 2.0;
	float cy = res.rows / 2.0;

	float scale_x = 1.1 * 2 * M_PI / res.cols;
	float scale_y = 1.1 * M_PI / res.rows;

	for (int i = 0; i < frames.size(); i++) {

		Eigen::Vector3f intrinsics = frames[i]->get_subsampled_intrinsics(0);
		Eigen::Quaternionf Qiw =
				frames[i]->get_position().unit_quaternion().inverse();

		Eigen::Matrix3f K;
		K << intrinsics[0], 0, intrinsics[1], 0, intrinsics[0], intrinsics[2], 0, 0, 1;
		Eigen::Matrix3f H = K * Qiw.matrix();

		for (int v = 0; v < map_x.rows; v++) {
			for (int u = 0; u < map_x.cols; u++) {

				float phi = (u - cx) * scale_x;
				float theta = (v - cy) * scale_y;

				Eigen::Vector3f vec(cos(theta) * cos(phi),
						-cos(theta) * sin(phi), -sin(theta));
				vec = H * vec;

				if (vec[2] > 0.01) {
					map_x.at<float>(v, u) = vec[0] / vec[2];
					map_y.at<float>(v, u) = vec[1] / vec[2];
				} else {
					map_x.at<float>(v, u) = -1;
					map_y.at<float>(v, u) = -1;
				}
			}
		}

		img_projected = 0.0f;
		cv::remap(frames[i]->intencity, img_projected, map_x, map_y,
				CV_INTER_LINEAR, cv::BORDER_TRANSPARENT, cv::Scalar(0, 0, 0));
		res += img_projected;
		cv::Mat mask;
		mask = (img_projected > 0);
		mask.convertTo(mask, CV_32F);
		w += mask;
	}

	return res / w;
}

void icp_map::align_z_axis() {

	pcl::PointCloud<pcl::PointXYZ>::Ptr point_cloud(
			new pcl::PointCloud<pcl::PointXYZ>);

	for (size_t i = 0; i < frames.size(); i++) {
		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud = frames[i]->get_pointcloud(
				-0.2, 0.2);

		*point_cloud += *cloud;
	}

	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);

	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
	// Create the segmentation object
	pcl::SACSegmentation<pcl::PointXYZ> seg;
	// Optional
	seg.setOptimizeCoefficients(true);
	// Mandatory
	seg.setModelType(pcl::SACMODEL_PLANE);
	seg.setMethodType(pcl::SAC_RANSAC);
	seg.setDistanceThreshold(0.02);
	seg.setProbability(0.99);
	seg.setMaxIterations(5000);

	seg.setInputCloud(point_cloud);
	seg.segment(*inliers, *coefficients);

	std::cerr << "Model coefficients: " << coefficients->values[0] << " "
			<< coefficients->values[1] << " " << coefficients->values[2] << " "
			<< coefficients->values[3] << " Num inliers "
			<< inliers->indices.size() << std::endl;

	Eigen::Affine3f transform = Eigen::Affine3f::Identity();
	if (coefficients->values[2] > 0) {
		transform.matrix().coeffRef(0, 2) = coefficients->values[0];
		transform.matrix().coeffRef(1, 2) = coefficients->values[1];
		transform.matrix().coeffRef(2, 2) = coefficients->values[2];
	} else {
		transform.matrix().coeffRef(0, 2) = -coefficients->values[0];
		transform.matrix().coeffRef(1, 2) = -coefficients->values[1];
		transform.matrix().coeffRef(2, 2) = -coefficients->values[2];
	}

	transform.matrix().col(0).head<3>() =
			transform.matrix().col(1).head<3>().cross(
					transform.matrix().col(2).head<3>());
	transform.matrix().col(1).head<3>() =
			transform.matrix().col(2).head<3>().cross(
					transform.matrix().col(0).head<3>());

	transform = transform.inverse();

	transform.matrix().coeffRef(2, 3) = coefficients->values[3];

	Sophus::SE3f t(transform.rotation(), transform.translation());

	for (size_t i = 0; i < frames.size(); i++) {
		frames[i]->get_position() = t * frames[i]->get_position();
	}

}

void icp_map::set_octomap(RmOctomapServer::Ptr & server) {

	for (size_t i = 0; i < frames.size(); i++) {
		pcl::PointCloud<pcl::PointXYZ>::Ptr point_cloud =
				frames[i]->get_pointcloud(0.1, 0.8);
		Eigen::Vector3f pos = frames[i]->get_position().translation();

		server->insertScan(tf::Point(pos[0], pos[1], pos[2]),
				pcl::PointCloud<pcl::PointXYZ>(), *point_cloud);
	}

	server->publishAll(ros::Time::now());
}

void icp_map::optimization_loop() {
	while (true) {
		if (frames.size() < 50) {
			sleep(5);
		} else {
			optimize();
		}
	}
}

void icp_map::save(const std::string & dir_name) {

	if (boost::filesystem::exists(dir_name)) {
		boost::filesystem::remove_all(dir_name);
	}

	boost::filesystem::create_directory(dir_name);
	boost::filesystem::create_directory(dir_name + "/rgb");
	boost::filesystem::create_directory(dir_name + "/depth");
	boost::filesystem::create_directory(dir_name + "/intencity");
	boost::filesystem::create_directory(dir_name + "/intencity_sub_1");
	boost::filesystem::create_directory(dir_name + "/intencity_sub_2");

	for (size_t i = 0; i < frames.size(); i++) {
		cv::imwrite(
				dir_name + "/rgb/" + boost::lexical_cast<std::string>(i)
						+ ".png", frames[i]->rgb);
		cv::imwrite(
				dir_name + "/depth/" + boost::lexical_cast<std::string>(i)
						+ ".png", frames[i]->depth);

		cv::imwrite(
				dir_name + "/intencity/" + boost::lexical_cast<std::string>(i)
						+ ".png", frames[i]->intencity * 255);

		cv::imwrite(
				dir_name + "/intencity_sub_1/"
						+ boost::lexical_cast<std::string>(i) + ".png",
				frames[i]->get_subsampled_intencity(1) * 255);

		cv::imwrite(
				dir_name + "/intencity_sub_2/"
						+ boost::lexical_cast<std::string>(i) + ".png",
				frames[i]->get_subsampled_intencity(2) * 255);

	}

	std::ofstream f((dir_name + "/positions.txt").c_str());
	for (size_t i = 0; i < frames.size(); i++) {
		Eigen::Quaternionf q = frames[i]->get_position().unit_quaternion();
		Eigen::Vector3f t = frames[i]->get_position().translation();
		f << q.x() << " " << q.y() << " " << q.z() << " " << q.w() << " "
				<< t.x() << " " << t.y() << " " << t.z() << std::endl;
	}

}

void icp_map::load(const std::string & dir_name) {

	std::vector<Sophus::SE3f> positions;

	std::ifstream f((dir_name + "/positions.txt").c_str());
	while (f) {
		Eigen::Quaternionf q;
		Eigen::Vector3f t;
		f >> q.x() >> q.y() >> q.z() >> q.w() >> t.x() >> t.y() >> t.z();
		positions.push_back(Sophus::SE3f(q, t));
	}

	positions.pop_back();
	std::cerr << "Loaded " << positions.size() << " positions" << std::endl;

	for (size_t i = 0; i < positions.size(); i++) {
		cv::Mat rgb = cv::imread(
				dir_name + "/rgb/" + boost::lexical_cast<std::string>(i)
						+ ".png", CV_LOAD_IMAGE_UNCHANGED);
		cv::Mat depth = cv::imread(
				dir_name + "/depth/" + boost::lexical_cast<std::string>(i)
						+ ".png", CV_LOAD_IMAGE_UNCHANGED);

		keyframe::Ptr k(
				new keyframe(rgb, depth, positions[i], intrinsics_vector, 0));
		frames.push_back(k);
	}

}

