// File: src/core/math/estimation/gradient_descent.cpp
/**
 * @file src/core/math/estimation/gradient_descent.cpp
 * @brief Implements truncated normal estimation using manual gradient descent.
 */
#include "TruncatedNormalEstimator.hpp"
#include <optional>
#include <vector>
#include <cmath>
#include <limits>

namespace DynaRange::Math::Estimation::Internal {

// Funciones normal_cdf, normal_pdf, normal_log_pdf (copiadas de dlib_optimizer.cpp o implementadas aquí)
inline double normal_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x * M_SQRT1_2));
}
inline double normal_log_pdf(double value, double mu, double sigma) {
    if (sigma <= 0) return -std::numeric_limits<double>::infinity();
    double z = (value - mu) / sigma;
    constexpr double log_inv_sqrt_2pi = -0.5 * std::log(2.0 * M_PI);
    return log_inv_sqrt_2pi - std::log(sigma) - 0.5 * z * z;
}

// Función objetivo NLL (similar a la de Rcpp)
double calculate_nll(const std::vector<double>& y, double trunc_point, double mu, double sigma) {
    if (sigma <= 1e-9) return std::numeric_limits<double>::infinity();

    double nll = 0.0;
    double z_trunc = (trunc_point - mu) / sigma;
    double log_cdf_trunc = std::log(normal_cdf(z_trunc) + 1e-16);

    for(double val : y) {
        if(val <= trunc_point) {
            nll -= log_cdf_trunc;
        } else {
            nll -= normal_log_pdf(val, mu, sigma);
        }
    }
     if (!std::isfinite(nll)) {
         return std::numeric_limits<double>::infinity();
     }
    return nll;
}

// --- Función principal de estimación con Descenso de Gradiente ---
std::optional<NormalParameters> EstimateWithGradientDescent(
    const std::vector<double>& truncated_data,
    double truncation_point,
    double mu_init,
    double sigma_init)
{
    double mu = mu_init;
    double sigma = sigma_init;

    // Parámetros del optimizador (ajustar según sea necesario)
    const int max_iter = 500;
    double lr_mu = 1e-3; // Learning rate para mu
    double lr_sigma = 1e-4; // Learning rate para sigma (suele necesitar ser más pequeño)
    const double tol = 1e-7; // Tolerancia para la convergencia
    const double h = 1e-5; // Step para diferencias finitas

    double prev_nll = calculate_nll(truncated_data, truncation_point, mu, sigma);
    if (!std::isfinite(prev_nll)) {
         // std::cerr << "Initial NLL is infinite. Check initial parameters or data." << std::endl;
         return std::nullopt;
    }


    for(int iter = 0; iter < max_iter; ++iter) {
        // Calcular gradientes usando diferencias finitas
        double nll_mu_plus = calculate_nll(truncated_data, truncation_point, mu + h, sigma);
        double nll_mu_minus = calculate_nll(truncated_data, truncation_point, mu - h, sigma);
        double grad_mu = (nll_mu_plus - nll_mu_minus) / (2.0 * h);

        double nll_sigma_plus = calculate_nll(truncated_data, truncation_point, mu, sigma + h);
        double nll_sigma_minus = calculate_nll(truncated_data, truncation_point, mu, sigma - h);
        double grad_sigma = (nll_sigma_plus - nll_sigma_minus) / (2.0 * h);

        // Actualizar parámetros (paso de descenso)
        double next_mu = mu - lr_mu * grad_mu;
        double next_sigma = sigma - lr_sigma * grad_sigma;

        // Proyección: Asegurar que sigma > 0
        if (next_sigma <= 1e-9) {
            next_sigma = 1e-9; // Establecer un mínimo pequeño para evitar problemas
            // Podríamos reducir lr_sigma aquí si golpeamos el límite a menudo
        }

        // Evaluar NLL con los nuevos parámetros
        double current_nll = calculate_nll(truncated_data, truncation_point, next_mu, next_sigma);

         // Verificar si la NLL empeoró (overshooting) o no es finita
         if (!std::isfinite(current_nll) || current_nll > prev_nll + tol * 10) { // Permitir un pequeño aumento por ruido numérico
             // Si empeora, reducir learning rates y no actualizar
             lr_mu *= 0.5;
             lr_sigma *= 0.5;
             // Opcional: Podríamos intentar de nuevo el paso con lr reducido en esta iteración
         } else {
             // Si mejora o es similar, aceptar el paso
             mu = next_mu;
             sigma = next_sigma;

             // Comprobar convergencia
             if (std::abs(prev_nll - current_nll) < tol) {
                 // std::cout << "Converged after " << iter + 1 << " iterations." << std::endl;
                 break; // Convergencia
             }
             prev_nll = current_nll;
             // Opcional: Aumentar ligeramente lr si la mejora es buena (más avanzado)
             // lr_mu *= 1.05; lr_sigma *= 1.05;
         }

         // Comprobar si los learning rates se han vuelto demasiado pequeños
         if (lr_mu < 1e-9 || lr_sigma < 1e-10) {
              // std::cerr << "Learning rates too small, stopping optimization." << std::endl;
              break;
         }

        // Debug: Imprimir estado cada N iteraciones
        // if (iter % 50 == 0) {
        //    std::cout << "Iter " << iter << ": mu=" << mu << ", sigma=" << sigma << ", NLL=" << prev_nll << ", lr_mu=" << lr_mu << ", lr_sig=" << lr_sigma << std::endl;
        // }

    } // Fin del bucle for

    // Validación final
    if (!std::isfinite(mu) || !std::isfinite(sigma) || sigma <= 0) {
        // std::cerr << "Optimization finished with invalid parameters." << std::endl;
        return std::nullopt;
    }


    return NormalParameters{mu, sigma};
}

} // namespace DynaRange::Math::Estimation::Internal
