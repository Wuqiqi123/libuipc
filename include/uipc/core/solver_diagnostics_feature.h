#pragma once

#include <uipc/core/feature.h>

namespace uipc::core
{
enum class SolverPipelineStage : U32
{
    None = 0,
    RebuildScene,
    PredictMotion,
    ComputeDyTopoEffect,
    SolveGlobalLinearSystem,
    LineSearch,
    UpdateVelocity,
};

enum class SolverFailureKind : U32
{
    None = 0,
    LineSearchLimit,
    NewtonLimit,
    LinearSystemLimit,
    LinearSystemBreakdown,
    NonFinite,
    Unexpected,
};

struct UIPC_CORE_API SolverDiagnostics
{
    SizeT              frame                         = 0;
    SolverPipelineStage stage                        = SolverPipelineStage::None;
    SolverFailureKind   failure_kind                 = SolverFailureKind::None;
    SizeT              newton_iterations             = 0;
    SizeT              line_search_iterations_total = 0;
    SizeT              line_search_iterations_max   = 0;
    SizeT              pcg_iterations_total         = 0;
    SizeT              pcg_iterations_max           = 0;
    SizeT              pcg_iterations_last          = 0;
    Float              pcg_relative_residual        = 0.0;
    Float              minimum_step_length          = 1.0;
    bool               newton_converged              = false;
    bool               linear_system_converged       = true;
    bool               strict_mode                   = false;
    bool               recovered                     = false;
    std::string        failure_message;
};

class UIPC_CORE_API SolverDiagnosticsFeatureOverrider
{
  public:
    virtual ~SolverDiagnosticsFeatureOverrider() = default;
    virtual SolverDiagnostics get_solver_diagnostics() const = 0;
};

class UIPC_CORE_API SolverDiagnosticsFeature final : public Feature
{
  public:
    constexpr static std::string_view FeatureName = "core/solver_diagnostics";

    explicit SolverDiagnosticsFeature(S<SolverDiagnosticsFeatureOverrider> overrider);

    SolverDiagnostics diagnostics() const;

  private:
    virtual std::string_view get_name() const override;
    S<SolverDiagnosticsFeatureOverrider> m_impl;
};
}  // namespace uipc::core
