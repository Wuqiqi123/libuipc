#include <engine/solver_feature.h>
#include <sim_engine.h>

namespace uipc::backend::cuda
{
SolverFeatureOverrider::SolverFeatureOverrider(SimEngine* engine)
    : m_engine(engine)
{
    UIPC_ASSERT(engine, "solver feature requires an engine");
}

core::SolverDiagnostics SolverFeatureOverrider::get_solver_diagnostics() const
{
    return m_engine->solver_diagnostics();
}

core::SolverRuntimeOptions SolverFeatureOverrider::get_solver_runtime_options() const
{
    return m_engine->solver_runtime_options();
}

void SolverFeatureOverrider::set_solver_runtime_options(
    const core::SolverRuntimeOptions& options)
{
    m_engine->set_solver_runtime_options(options);
}
}  // namespace uipc::backend::cuda
