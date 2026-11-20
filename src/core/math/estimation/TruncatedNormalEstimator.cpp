// File: src/core/math/estimation/TruncatedNormalEstimator.cpp
/**
 * @file src/core/math/estimation/TruncatedNormalEstimator.cpp
 * @brief Implements the dispatcher for truncated normal parameter estimation.
 */
#include "TruncatedNormalEstimator.hpp"
#include "Constants.hpp"                 
#include "../../math/Math.hpp"          // Para CalculateMean
#include <numeric>                      // Para std::accumulate
#include <cmath>                        // Para std::sqrt
#include <vector>                       // Para std::vector
#include <algorithm>                    // Para std::max

// --- Declaraciones adelantadas de las implementaciones específicas ---
// Estas funciones DEBEN estar definidas en sus respectivos .cpp
namespace DynaRange::Math::Estimation::Internal {

// Declaración de la función que usará LBFGSpp (definida en lbfgspp_optimizer.cpp)
std::optional<NormalParameters> EstimateWithLBFGSpp(
    const std::vector<double>& truncated_data,
    double truncation_point,
    double mu_init,
    double sigma_init);

// Declaración de la función que usará descenso de gradiente (definida en gradient_descent.cpp)
std::optional<NormalParameters> EstimateWithGradientDescent(
    const std::vector<double>& truncated_data,
    double truncation_point,
    double mu_init,
    double sigma_init);

// Función auxiliar para calcular stddev (si no existe en Math.cpp)
double CalculateStdDev(const std::vector<double>& data, double mean) {
    if (data.size() < 2) return 0.0;
    double sq_sum = std::accumulate(data.begin(), data.end(), 0.0,
                                    [mean](double accumulator, const double& val) {
                                        return accumulator + (val - mean) * (val - mean);
                                    });
    // Usar N en lugar de N-1 para consistencia con cv::meanStdDev sin flags
     return std::sqrt(sq_sum / data.size()); // Estimador sesgado (como OpenCV por defecto)
}

/**
 * @brief Estimates mu and sigma of a normal distribution left-censored at
 * 'censor_point' (every value <= censor_point is reported as clipped) using EM.
 * @details Each iteration replaces the clipped values by their conditional
 * expectations E[X | X <= c] and E[X^2 | X <= c] and recomputes mu and sigma.
 * It needs no step sizes and is invariant to the scale of the data.
 */
std::optional<NormalParameters> EstimateCensoredNormalEM(
    const std::vector<double>& data,
    double censor_point)
{
    const double n = static_cast<double>(data.size());
    double k = 0.0, sum1 = 0.0, sum2 = 0.0, all1 = 0.0, all2 = 0.0;
    for (double v : data) {
        all1 += v; all2 += v * v;
        if (v <= censor_point) { k += 1.0; }
        else { sum1 += v; sum2 += v * v; }
    }
    if (n - k < 2.0) return std::nullopt; // not enough unclipped values

    double mu = all1 / n;
    double var = all2 / n - mu * mu;
    if (k == 0.0) { // nothing clipped: plain estimates
        if (var <= 0.0) return std::nullopt;
        return NormalParameters{mu, std::sqrt(var)};
    }
    if (var <= 0.0) var = 1e-12;
    double sigma = std::sqrt(var);

    constexpr double kInvSqrt2Pi = 0.3989422804014327;
    for (int iter = 0; iter < 20000; ++iter) {
        const double a = (censor_point - mu) / sigma;
        const double Phi = 0.5 * std::erfc(-a * M_SQRT1_2);
        const double phi = kInvSqrt2Pi * std::exp(-0.5 * a * a);
        const double lambda = phi / std::max(Phi, 1e-300);
        const double e1 = mu - sigma * lambda;
        const double e2 = mu * mu + sigma * sigma - sigma * lambda * (censor_point + mu);

        const double new_mu = (sum1 + k * e1) / n;
        const double new_var = (sum2 + k * e2) / n - new_mu * new_mu;
        if (!std::isfinite(new_mu) || !std::isfinite(new_var) || new_var <= 0.0) return std::nullopt;
        const double new_sigma = std::sqrt(new_var);

        const bool converged = std::abs(new_mu - mu) < 1e-9 * new_sigma &&
                               std::abs(new_sigma - sigma) < 1e-9 * new_sigma;
        mu = new_mu;
        sigma = new_sigma;
        if (converged) break;
    }
    if (!std::isfinite(mu) || !std::isfinite(sigma) || sigma <= 0.0) return std::nullopt;
    return NormalParameters{mu, sigma};
}

} // namespace DynaRange::Math::Estimation::Internal


namespace DynaRange::Math::Estimation {

std::optional<NormalParameters> EstimateTruncatedNormal(
    const std::vector<double>& truncated_data,
    double truncation_point,
    double initial_mu,
    double initial_sigma)
{
    if (truncated_data.size() < 3) {
        return std::nullopt;
    }

    if constexpr (USE_EM_ESTIMATOR) {
        return Internal::EstimateCensoredNormalEM(truncated_data, truncation_point);
    }

    double mu_init = initial_mu;
    double sigma_init = initial_sigma;

    if (mu_init < 0.0 || sigma_init < 0.0) {
        std::vector<double> above_trunc;
        for(double val : truncated_data) {
            if (val > truncation_point) {
                above_trunc.push_back(val);
            }
        }
        if (above_trunc.size() < 2) {
             if (truncated_data.size() >=2) {
                 mu_init = CalculateMean(truncated_data);
                 sigma_init = Internal::CalculateStdDev(truncated_data, mu_init);
             } else {
                 return std::nullopt;
             }
        } else {
             mu_init = CalculateMean(above_trunc);
             sigma_init = Internal::CalculateStdDev(above_trunc, mu_init);
        }

        if (sigma_init <= 1e-9) {
             if (mu_init > 1e-9) sigma_init = mu_init * 0.01;
             else sigma_init = 1e-6;
        }
    }

    // Llamar a la implementación seleccionada por la constante en Constants.hpp
    if constexpr (USE_LBFGSPP_ESTIMATOR) { // 'if constexpr' para optimización en tiempo de compilación
        return Internal::EstimateWithLBFGSpp(truncated_data, truncation_point, mu_init, sigma_init);
    } else {
        return Internal::EstimateWithGradientDescent(truncated_data, truncation_point, mu_init, sigma_init);
    }
}

} // namespace DynaRange::Math::Estimation