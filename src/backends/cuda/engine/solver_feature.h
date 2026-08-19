#pragma once

#include <uipc/core/solver_control_feature.h>
#include <uipc/core/solver_diagnostics_feature.h>

namespace uipc::backend::cuda
{
class SimEngine;

class SolverFeatureOverrider final
    : public core::SolverDiagnosticsFeatureOverrider,
      public core::SolverControlFeatureOverrider
{
  public:
    explicit SolverFeatureOverrider(SimEngine* engine);

  private:
    core::SolverDiagnostics get_solver_diagnostics() const override;
    core::SolverRuntimeOptions get_solver_runtime_options() const override;
    void set_solver_runtime_options(const core::SolverRuntimeOptions& options) override;

    SimEngine* m_engine = nullptr;
};
}  // namespace uipc::backend::cuda
