// File: src/core/math/estimation/Constants.hpp
/**
 * @file src/core/math/estimation/Constants.hpp
 * @brief Defines constants for the estimation module.
 */
#pragma once

namespace DynaRange::Math::Estimation {

    /**
     * @brief Selects the algorithm for truncated normal estimation.
     * @details
     * - true: Use LBFGSpp (requires Eigen + LBFGSpp headers).
     * - false: Use manual Gradient Descent.
     */
    constexpr bool USE_LBFGSPP_ESTIMATOR = false; // <-- CAMBIA AQUÍ para seleccionar (true = LBFGSpp, false = GD)

    /**
     * @brief Uses the Expectation-Maximisation (EM) estimator for the censored normal.
     * @details EM is scale-invariant and has no learning rates, so it works both
     * with very small noise in normalised units (e.g. Nikon ISO 100, sigma ~ 4e-5)
     * and with large noise. When true, USE_LBFGSPP_ESTIMATOR is ignored.
     */
    constexpr bool USE_EM_ESTIMATOR = true;

} // namespace DynaRange::Math::Estimation