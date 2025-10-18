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

} // namespace DynaRange::Math::Estimation