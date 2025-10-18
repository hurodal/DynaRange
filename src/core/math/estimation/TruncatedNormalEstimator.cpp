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