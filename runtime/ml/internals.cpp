#include "internals.h"

#include <cmath>

static_assert(sizeof(ml_internals::XgbTreeNode) == 8, "unexpected sizeof(XgbTreeNode)");

namespace ml_internals {

[[gnu::always_inline]] static inline float transform_base_score(XGTrainParamObjective tparam_objective, float base_score) {
  switch (tparam_objective) {
    case XGTrainParamObjective::binary_logistic:
      return -logf(1.0f / base_score - 1.0f);
    case XGTrainParamObjective::rank_pairwise:
      return base_score;
    default:
      __builtin_unreachable();
  }
}

[[gnu::always_inline]] static inline double transform_prediction(XGTrainParamObjective tparam_objective, double score) {
  switch (tparam_objective) {
    case XGTrainParamObjective::binary_logistic:
      return 1.0 / (1.0 + std::exp(-score));
    case XGTrainParamObjective::rank_pairwise:
      return score;
    default:
      __builtin_unreachable();
  }
}

float XgbModel::transform_base_score() const noexcept {
  return ml_internals::transform_base_score(tparam_objective, base_score);
}

double XgbModel::transform_prediction(double score) const noexcept {
  score = ml_internals::transform_prediction(tparam_objective, score);

  switch (calibration.calibration_method) {
    case CalibrationMethod::platt_scaling:
      if (tparam_objective == XGTrainParamObjective::binary_logistic) {
        score = -log(1. / score - 1);
      }
      return 1 / (1. + exp(-(calibration.platt_slope * score + calibration.platt_intercept)));
    default:
      return score;
  }
}

} // namespace ml_internals
