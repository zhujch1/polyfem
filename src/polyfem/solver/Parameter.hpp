#pragma once

#include <polyfem/State.hpp>

namespace polyfem
{
	class Parameter
	{
	public:
		Parameter(std::vector<std::shared_ptr<State>> states_ptr) : states_ptr_(states_ptr) {}
		virtual ~Parameter() = default;

		inline const State& get_state() const { return *(states_ptr_[0]); }

		inline virtual bool contains_state(const State &state) const
		{
			for (auto s : states_ptr_)
				if (s.get() == &state)
					return true;
			return false;
		}

		virtual void update() = 0;

		virtual void map(const Eigen::MatrixXd &x, Eigen::MatrixXd &q) { q = x; }
		virtual Eigen::VectorXd map_grad(const Eigen::VectorXd &full_grad) const { return full_grad; }

		virtual void smoothing(const Eigen::VectorXd &x, Eigen::VectorXd &new_x) { new_x = x; }
		virtual bool is_step_valid(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) { return true; }
		virtual bool is_intersection_free(const Eigen::VectorXd &x) { return true; }
		virtual bool is_step_collision_free(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) { return true; }
		virtual double max_step_size(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) { return 1; }

		virtual void line_search_begin(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) {}
		virtual void line_search_end(bool failed) {}
		virtual void post_step(const int iter_num, const Eigen::VectorXd &x0) {}

		inline virtual void set_optimization_dim(const int optimization_dim) final { optimization_dim_ = optimization_dim; }
		inline virtual int full_dim() const // dim before projection
		{
			return full_dim_;
		}
		inline virtual int optimization_dim() final
		{
			if (optimization_dim_ <= 0)
				log_and_throw_error("Invalid optimization dimension!");
			return optimization_dim_;
		}

		inline virtual std::string name() const { return parameter_name_; }

		virtual bool pre_solve(const Eigen::VectorXd &newX) { return true; }
		virtual void post_solve(const Eigen::VectorXd &newX) {}

		inline virtual bool remesh(Eigen::VectorXd &x) { return true; };

		virtual Eigen::VectorXd force_inequality_constraint(const Eigen::VectorXd &x0, const Eigen::VectorXd &dx) { return x0 + dx; }
		virtual int n_inequality_constraints() { return 0; }
		virtual double inequality_constraint_val(const Eigen::VectorXd &x, const int index)
		{
			assert(false);
			return std::nan("");
		}
		virtual Eigen::VectorXd inequality_constraint_grad(const Eigen::VectorXd &x, const int index)
		{
			assert(false);
			return Eigen::VectorXd();
		}

		virtual Eigen::VectorXd get_lower_bound(const Eigen::VectorXd &x) const
		{
			Eigen::VectorXd min(x.size());
			min.setConstant(std::numeric_limits<double>::min());
			return min;
		}
		virtual Eigen::VectorXd get_upper_bound(const Eigen::VectorXd &x) const
		{
			Eigen::VectorXd max(x.size());
			max.setConstant(std::numeric_limits<double>::max());
			return max;
		}

	protected:
		std::vector<std::shared_ptr<State>> states_ptr_;
		int optimization_dim_;
		int full_dim_;
		std::string parameter_name_;
		double max_change_;
	};
} // namespace polyfem