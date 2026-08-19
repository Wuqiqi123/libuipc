#include <uipc/core/solver_diagnostics_feature.h>

namespace uipc::core
{
SolverDiagnosticsFeature::SolverDiagnosticsFeature(
    S<SolverDiagnosticsFeatureOverrider> overrider)
    : m_impl(std::move(overrider))
{
}

SolverDiagnostics SolverDiagnosticsFeature::diagnostics() const
{
    return m_impl->get_solver_diagnostics();
}

std::string_view SolverDiagnosticsFeature::get_name() const
{
    return FeatureName;
}
}  // namespace uipc::core
