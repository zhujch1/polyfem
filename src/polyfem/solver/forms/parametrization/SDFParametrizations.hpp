#pragma once

#include "Parametrization.hpp"
#include <polyfem/Common.hpp>

namespace polyfem::solver
{
    class SDF2Mesh : public Parametrization
    {
    public:
        SDF2Mesh(const std::string wire_path, const std::string out_path, const json &opts) : wire_path_(wire_path), out_path_(out_path), opts_(opts) {}

        int size(const int x_size) const override;

        Eigen::VectorXd eval(const Eigen::VectorXd &x) const override;
        Eigen::VectorXd apply_jacobian(const Eigen::VectorXd &grad, const Eigen::VectorXd &x) const override;
    
    private:
        bool isosurface_inflator(const Eigen::VectorXd &x) const;

        const std::string wire_path_, out_path_;
        const json opts_;
        
        mutable Eigen::VectorXd last_x;
        mutable Eigen::MatrixXd Vout, vertex_normals, shape_vel;
        mutable Eigen::MatrixXi Fout;
    };

    class MeshTiling : public Parametrization
    {
    public:
        MeshTiling(const Eigen::VectorXi &nums, const std::string in_path, const std::string out_path);

        int size(const int x_size) const override;

        Eigen::VectorXd eval(const Eigen::VectorXd &x) const override;
        Eigen::VectorXd apply_jacobian(const Eigen::VectorXd &grad, const Eigen::VectorXd &x) const override;
    
    private:
        const Eigen::VectorXi nums_;
        const std::string in_path_, out_path_;

        bool tiling(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, Eigen::MatrixXd &Vnew, Eigen::MatrixXi &Fnew) const;

        mutable Eigen::MatrixXd last_V;
        mutable Eigen::VectorXi index_map;
    };

    class MeshAffine : public Parametrization
    {
    public:
        MeshAffine(const Eigen::MatrixXd &A, const Eigen::VectorXd &b, const std::string in_path, const std::string out_path);

        int size(const int x_size) const override { return x_size; }

        Eigen::VectorXd eval(const Eigen::VectorXd &x) const override;
        Eigen::VectorXd apply_jacobian(const Eigen::VectorXd &grad, const Eigen::VectorXd &x) const override;
    
    private:
        const Eigen::MatrixXd A_;
        const Eigen::VectorXd b_;
        const std::string in_path_, out_path_;

        mutable Eigen::VectorXd last_x;
    };
}