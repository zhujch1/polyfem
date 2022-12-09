#include "ElasticityUtils.hpp"
#include <polyfem/utils/Logger.hpp>

#include <tinyexpr.h>

namespace polyfem
{
	using namespace assembler;
	using namespace basis;
	using namespace utils;

	Eigen::VectorXd gradient_from_energy(const int size, const int n_bases, const assembler::NonLinearAssemblerData &data,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 6, 1>>(const assembler::NonLinearAssemblerData &)> &fun6,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 8, 1>>(const assembler::NonLinearAssemblerData &)> &fun8,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 12, 1>>(const assembler::NonLinearAssemblerData &)> &fun12,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 18, 1>>(const assembler::NonLinearAssemblerData &)> &fun18,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 24, 1>>(const assembler::NonLinearAssemblerData &)> &fun24,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 30, 1>>(const assembler::NonLinearAssemblerData &)> &fun30,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 60, 1>>(const assembler::NonLinearAssemblerData &)> &fun60,
										 const std::function<DScalar1<double, Eigen::Matrix<double, 81, 1>>(const assembler::NonLinearAssemblerData &)> &fun81,
										 const std::function<DScalar1<double, Eigen::Matrix<double, Eigen::Dynamic, 1, 0, SMALL_N, 1>>(const assembler::NonLinearAssemblerData &)> &funN,
										 const std::function<DScalar1<double, Eigen::Matrix<double, Eigen::Dynamic, 1, 0, BIG_N, 1>>(const assembler::NonLinearAssemblerData &)> &funBigN,
										 const std::function<DScalar1<double, Eigen::VectorXd>(const assembler::NonLinearAssemblerData &)> &funn)
	{
		Eigen::VectorXd grad;

		switch (size * n_bases)
		{
		case 6:
		{
			auto auto_diff_energy = fun6(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 8:
		{
			auto auto_diff_energy = fun8(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 12:
		{
			auto auto_diff_energy = fun12(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 18:
		{
			auto auto_diff_energy = fun18(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 24:
		{
			auto auto_diff_energy = fun24(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 30:
		{
			auto auto_diff_energy = fun30(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 60:
		{
			auto auto_diff_energy = fun60(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		case 81:
		{
			auto auto_diff_energy = fun81(data);
			grad = auto_diff_energy.getGradient();
			break;
		}
		}

		if (grad.size() <= 0)
		{
			if (n_bases * size <= SMALL_N)
			{
				auto auto_diff_energy = funN(data);
				grad = auto_diff_energy.getGradient();
			}
			else if (n_bases * size <= BIG_N)
			{
				auto auto_diff_energy = funBigN(data);
				grad = auto_diff_energy.getGradient();
			}
			else
			{
				static bool show_message = true;

				if (show_message)
				{
					logger().debug("[Warning] grad {}^{} not using static sizes", n_bases, size);
					show_message = false;
				}

				auto auto_diff_energy = funn(data);
				grad = auto_diff_energy.getGradient();
			}
		}

		return grad;
	}

	Eigen::MatrixXd hessian_from_energy(const int size, const int n_bases, const assembler::NonLinearAssemblerData &data,
										const std::function<DScalar2<double, Eigen::Matrix<double, 6, 1>, Eigen::Matrix<double, 6, 6>>(const assembler::NonLinearAssemblerData &)> &fun6,
										const std::function<DScalar2<double, Eigen::Matrix<double, 8, 1>, Eigen::Matrix<double, 8, 8>>(const assembler::NonLinearAssemblerData &)> &fun8,
										const std::function<DScalar2<double, Eigen::Matrix<double, 12, 1>, Eigen::Matrix<double, 12, 12>>(const assembler::NonLinearAssemblerData &)> &fun12,
										const std::function<DScalar2<double, Eigen::Matrix<double, 18, 1>, Eigen::Matrix<double, 18, 18>>(const assembler::NonLinearAssemblerData &)> &fun18,
										const std::function<DScalar2<double, Eigen::Matrix<double, 24, 1>, Eigen::Matrix<double, 24, 24>>(const assembler::NonLinearAssemblerData &)> &fun24,
										const std::function<DScalar2<double, Eigen::Matrix<double, 30, 1>, Eigen::Matrix<double, 30, 30>>(const assembler::NonLinearAssemblerData &)> &fun30,
										const std::function<DScalar2<double, Eigen::Matrix<double, 60, 1>, Eigen::Matrix<double, 60, 60>>(const assembler::NonLinearAssemblerData &)> &fun60,
										const std::function<DScalar2<double, Eigen::Matrix<double, 81, 1>, Eigen::Matrix<double, 81, 81>>(const assembler::NonLinearAssemblerData &)> &fun81,
										const std::function<DScalar2<double, Eigen::Matrix<double, Eigen::Dynamic, 1, 0, SMALL_N, 1>, Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, 0, SMALL_N, SMALL_N>>(const assembler::NonLinearAssemblerData &)> &funN,
										const std::function<DScalar2<double, Eigen::VectorXd, Eigen::MatrixXd>(const assembler::NonLinearAssemblerData &)> &funn)
	{
		Eigen::MatrixXd hessian;

		switch (size * n_bases)
		{
		case 6:
		{
			auto auto_diff_energy = fun6(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 8:
		{
			auto auto_diff_energy = fun8(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 12:
		{
			auto auto_diff_energy = fun12(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 18:
		{
			auto auto_diff_energy = fun18(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 24:
		{
			auto auto_diff_energy = fun24(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 30:
		{
			auto auto_diff_energy = fun30(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 60:
		{
			auto auto_diff_energy = fun60(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		case 81:
		{
			auto auto_diff_energy = fun81(data);
			hessian = auto_diff_energy.getHessian();
			break;
		}
		}

		if (hessian.size() <= 0)
		{
			if (n_bases * size <= SMALL_N)
			{
				auto auto_diff_energy = funN(data);
				hessian = auto_diff_energy.getHessian();
			}
			else
			{
				static bool show_message = true;

				if (show_message)
				{
					logger().debug("[Warning] hessian {}*{} not using static sizes", n_bases, size);
					show_message = false;
				}

				auto auto_diff_energy = funn(data);
				hessian = auto_diff_energy.getHessian();
			}
		}

		// time.stop();
		// std::cout << "-- hessian: " << time.getElapsedTime() << std::endl;

		return hessian;
	}

	void compute_diplacement_grad(const int size, const ElementBases &bs, const ElementAssemblyValues &vals, const Eigen::MatrixXd &local_pts, const int p, const Eigen::MatrixXd &displacement, Eigen::MatrixXd &displacement_grad)
	{
		assert(displacement.cols() == 1);

		displacement_grad.setZero();

		for (std::size_t j = 0; j < bs.bases.size(); ++j)
		{
			const Basis &b = bs.bases[j];
			const auto &loc_val = vals.basis_values[j];

			assert(bs.bases.size() == vals.basis_values.size());
			assert(loc_val.grad.rows() == local_pts.rows());
			assert(loc_val.grad.cols() == size);

			for (int d = 0; d < size; ++d)
			{
				for (std::size_t ii = 0; ii < b.global().size(); ++ii)
				{
					displacement_grad.row(d) += b.global()[ii].val * loc_val.grad.row(p) * displacement(b.global()[ii].index * size + d);
				}
			}
		}

		displacement_grad = (displacement_grad * vals.jac_it[p]).eval();
	}

	double von_mises_stress_for_stress_tensor(const Eigen::MatrixXd &stress)
	{
		double von_mises_stress;

		if (stress.rows() == 3)
		{
			von_mises_stress = 0.5 * (stress(0, 0) - stress(1, 1)) * (stress(0, 0) - stress(1, 1)) + 3.0 * stress(0, 1) * stress(1, 0);
			von_mises_stress += 0.5 * (stress(2, 2) - stress(1, 1)) * (stress(2, 2) - stress(1, 1)) + 3.0 * stress(2, 1) * stress(2, 1);
			von_mises_stress += 0.5 * (stress(2, 2) - stress(0, 0)) * (stress(2, 2) - stress(0, 0)) + 3.0 * stress(2, 0) * stress(2, 0);
		}
		else
		{
			// von_mises_stress = ( stress(0, 0) - stress(1, 1) ) * ( stress(0, 0) - stress(1, 1) ) + 3.0  *  stress(0, 1) * stress(1, 0);
			von_mises_stress = stress(0, 0) * stress(0, 0) - stress(0, 0) * stress(1, 1) + stress(1, 1) * stress(1, 1) + 3.0 * stress(0, 1) * stress(1, 0);
		}

		von_mises_stress = sqrt(fabs(von_mises_stress));

		return von_mises_stress;
	}

} // namespace polyfem