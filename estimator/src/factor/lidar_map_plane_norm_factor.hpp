#include <ceres/ceres.h>
#include <ceres/rotation.h>

#include <eigen3/Eigen/Dense>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl_conversions/pcl_conversions.h>

// calculate distrance from point to plane (using normal)
struct LidarMapPlaneNormFactor
{
	LidarMapPlaneNormFactor(const Eigen::Vector3d &point, const Eigen::Vector4d &coeff, const Eigen::Matrix3d &cov_matrix = Eigen::Matrix3f::Identity())
		: point_(point), coeff_(coeff), cov_matrix_(cov_matrix)
	{
		sqrt_info_ = Eigen::LLT<Eigen::Matrix<double, 3, 3> >(cov_matrix_.inverse()).matrixL().transpose();
	}

	template <typename T>
	bool operator()(const T *param, T *residuals_ptr) const
	{
		Eigen::Quaternion<T> q_w_curr(param[6], param[3], param[4], param[5]);
		Eigen::Matrix<T, 3, 1> t_w_curr(param[0], param[1], param[2]);

		Eigen::Matrix<T, 3, 1> w(T(coeff_(0)), T(coeff_(1)), T(coeff_(2)));
		Eigen::Matrix<T, 3, 1> cp(T(point_(0)), T(point_(1)), T(point_(2)));
		T d = T(coeff_(3));
		T a = w.dot(q_w_curr * cp + t_w_curr) + d;
		Eigen::Map<Eigen::Matrix<T, 3, 1> > residuals(residuals_ptr);
		residuals.template block<3, 1>(0, 0) = a * w;
		residuals.applyOnTheLeft(sqrt_info_.template cast<T>());
		return true;
	}

	static ceres::CostFunction *Create(const Eigen::Vector3d &point, const Eigen::Vector4d &coeff, const Eigen::Matrix3d &cov_matrix)
	{
		return (new ceres::AutoDiffCostFunction<
				LidarMapPlaneNormFactor, 3, 7>(new LidarMapPlaneNormFactor(point, coeff, cov_matrix)));
	}

	Eigen::Vector3d point_;
	Eigen::Vector4d coeff_;
	Eigen::Matrix3d cov_matrix_, sqrt_info_;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
