// File: src/core/math/estimation/lbfgspp_optimizer.cpp
/**
 * @file src/core/math/estimation/lbfgspp_optimizer.cpp
 * @brief Implements truncated normal estimation using the LBFGSpp library.
 */
#include "TruncatedNormalEstimator.hpp" // Para NormalParameters
#include "../../../core/DebugConfig.hpp" // Para la macro de selección

#if USE_LBFGSPP_ESTIMATOR == 1 // Compilar solo si está activado

#include <LBFGS.h> // Incluir LBFGSpp directamente 
#include <Eigen/Core>      // Incluir Eigen Core
#include <vector>
#include <cmath> // Para std::log, std::sqrt, M_SQRT1_2, M_PI, std::erf, std::exp
#include <limits> // Para std::numeric_limits

// Definir constantes matemáticas si no están disponibles
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440 // 1/sqrt(2)
#endif

namespace DynaRange::Math::Estimation::Internal {

// --- Funciones auxiliares para la distribución normal (igual que antes) ---
inline double normal_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x * M_SQRT1_2));
}
inline double normal_log_pdf(double value, double mu, double sigma) {
    if (sigma <= 1e-9) return -std::numeric_limits<double>::infinity(); // Evitar log(0) o división por cero
    double z = (value - mu) / sigma;
    constexpr double log_inv_sqrt_2pi = -0.5 * std::log(2.0 * M_PI);
    // Asegurarse de que sigma no sea cero antes de calcular el logaritmo
    double log_sigma_term = (sigma > 1e-16) ? std::log(sigma) : std::log(1e-16);
    return log_inv_sqrt_2pi - log_sigma_term - 0.5 * z * z;
}
// PDF normal estándar (phi minúscula) - necesaria para el gradiente
inline double normal_std_pdf(double z) {
    constexpr double inv_sqrt_2pi = 1.0 / std::sqrt(2.0 * M_PI);
    return inv_sqrt_2pi * std::exp(-0.5 * z * z);
}


// --- Clase para la función objetivo (Negative Log-Likelihood) para LBFGSpp ---
class NLLObjectiveLBFGSpp {
public:
    // LBFGSpp requiere typedefs para el tipo escalar y el vector
    using Scalar = double;
    using Vector = Eigen::VectorXd;

    NLLObjectiveLBFGSpp(const std::vector<double>& data, double trunc_pt)
        : truncated_data(data), truncation_point(trunc_pt) {}

    // Evalúa la NLL dado un vector de parámetros [mu, log(sigma)]
    Scalar operator()(const Vector& params, Vector& grad) {
        Scalar mu = params[0];
        Scalar log_sigma = params[1];
        Scalar sigma = std::exp(log_sigma);

        // Clamping para evitar sigma <= 0
        if (sigma <= 1e-9) {
            sigma = 1e-9;
        }

        Scalar nll = 0.0;
        Scalar z_trunc = (truncation_point - mu) / sigma;
        Scalar cdf_trunc = normal_cdf(z_trunc);
        Scalar pdf_trunc = normal_std_pdf(z_trunc); // PDF normal estándar en z_trunc

        // Evitar log(0) o división por cero en CDF
        Scalar log_cdf_trunc = (cdf_trunc > 1e-16) ? std::log(cdf_trunc) : std::log(1e-16);
        Scalar pdf_over_cdf = (cdf_trunc > 1e-16) ? (pdf_trunc / cdf_trunc) : (pdf_trunc / 1e-16);

        // Calcular la NLL y los gradientes simultáneamente
        grad.setZero(2); // Inicializar gradiente a [0, 0]
        int n_cens = 0; // Contador de puntos censurados

        for (Scalar val : truncated_data) {
            if (val <= truncation_point) {
                // --- Punto censurado ---
                n_cens++;
                nll -= log_cdf_trunc; // Restar log(CDF)

                // Derivada de log(CDF(z)) respecto a mu = -pdf(z) / (sigma * CDF(z))
                grad[0] += pdf_over_cdf / sigma; // Sumar -(-derivada) = +derivada
                // Derivada de log(CDF(z)) respecto a log(sigma) = -z * pdf(z) / CDF(z)
                grad[1] += z_trunc * pdf_over_cdf; // Sumar -(-derivada) = +derivada

            } else {
                // --- Punto no censurado ---
                double log_pdf_val = normal_log_pdf(val, mu, sigma);
                nll -= log_pdf_val; // Restar log(PDF)

                // Derivada de log(PDF) respecto a mu = (val - mu) / sigma^2
                grad[0] -= (val - mu) / (sigma * sigma); // Sumar -derivada
                // Derivada de log(PDF) respecto a log(sigma) = [(val - mu)^2 / sigma^2] - 1 = z^2 - 1
                double z_val = (val - mu) / sigma;
                grad[1] -= (z_val * z_val - 1.0); // Sumar -derivada
            }
        }

        // Devolver Inf si NLL no es finita
        if (!std::isfinite(nll)) {
            return std::numeric_limits<Scalar>::infinity();
        }
        return nll;
    }

private:
    const std::vector<double>& truncated_data;
    double truncation_point;
};

// --- Función principal de estimación con LBFGSpp ---
std::optional<NormalParameters> EstimateWithLBFGSpp(
    const std::vector<double>& truncated_data,
    double truncation_point,
    double mu_init,
    double sigma_init)
{
    // Asegurar sigma inicial positivo
    if (sigma_init <= 1e-9) {
        sigma_init = 1e-9;
    }

    try {
        // Configurar parámetros del optimizador
        LBFGSpp::LBFGSParam<double> param;
        param.epsilon = 1e-6; // Tolerancia para la convergencia del gradiente
        param.max_iterations = 100; // Máximo de iteraciones

        // Crear el solver
        NLLObjectiveLBFGSpp objective(truncated_data, truncation_point);
        LBFGSpp::LBFGSSolver<double> solver(param);

        // Vector de parámetros iniciales: [mu, log(sigma)]
        Eigen::VectorXd x(2);
        x[0] = mu_init;
        x[1] = std::log(sigma_init);

        // Verificar NLL inicial
        Eigen::VectorXd initial_grad(2); // Necesitamos un vector para el gradiente inicial
        double initial_nll = objective(x, initial_grad);
        if (!std::isfinite(initial_nll)) {
             // std::cerr << "Warning: Initial parameters result in non-finite NLL for LBFGSpp." << std::endl;
             return std::nullopt;
        }

        // Ejecutar la minimización
        double final_nll; // Para almacenar el valor final de la NLL
        int niter = solver.minimize(objective, x, final_nll);

        // Extraer resultados
        double final_mu = x[0];
        double final_sigma = std::exp(x[1]);

        // Validación simple
        if (!std::isfinite(final_mu) || !std::isfinite(final_sigma) || final_sigma <= 1e-9) {
            return std::nullopt;
        }

        return NormalParameters{final_mu, final_sigma};

    } catch (const std::exception& e) {
        // Loggear el error si se desea
        // std::cerr << "LBFGSpp optimization failed: " << e.what() << std::endl;
        return std::nullopt;
    } catch (...) {
        // std::cerr << "Unknown error during LBFGSpp optimization." << std::endl;
        return std::nullopt;
    }
}

} // namespace DynaRange::Math::Estimation::Internal

#endif // USE_LBFGSPP_ESTIMATOR == 1