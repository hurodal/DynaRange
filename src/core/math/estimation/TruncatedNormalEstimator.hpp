// File: src/core/math/estimation/TruncatedNormalEstimator.hpp
/**
 * @file src/core/math/estimation/TruncatedNormalEstimator.hpp
 * @brief Declares the function to estimate parameters of a left-truncated normal distribution.
 */
#pragma once

#include <vector>
#include <optional>

namespace DynaRange::Math::Estimation {

/**
 * @struct NormalParameters
 * @brief Holds the estimated parameters (mean and standard deviation) of a normal distribution.
 */
struct NormalParameters {
    double mu;     ///< Estimated mean (μ).
    double sigma;  ///< Estimated standard deviation (σ).
};

/**
 * @brief Estimates the original mean (mu) and standard deviation (sigma)
 * of a normal distribution given data that has been left-truncated.
 * @param truncated_data A vector containing the observed data points, where values
 * below the truncation point have been set to that point.
 * @param truncation_point The value below which the original data was truncated (e.g., 0.0).
 * @param initial_mu An initial guess for the mean. If negative, calculated from data mean.
 * @param initial_sigma An initial guess for the standard deviation. If negative, calculated from data stddev.
 * @return An std::optional containing the estimated NormalParameters on success,
 * or std::nullopt if estimation fails (e.g., insufficient data, non-convergence).
 */
std::optional<NormalParameters> EstimateTruncatedNormal(
    const std::vector<double>& truncated_data,
    double truncation_point,
    double initial_mu = -1.0,
    double initial_sigma = -1.0
);

} // namespace DynaRange::Math::Estimation