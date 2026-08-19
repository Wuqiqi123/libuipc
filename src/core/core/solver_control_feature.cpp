#include <uipc/core/solver_control_feature.h>
#include <uipc/common/uipc.h>

namespace uipc::core
{
SolverControlFeature::SolverControlFeature(S<SolverControlFeatureOverrider> overrider)
    : m_impl(std::move(overrider))
{
}

SolverRuntimeOptions SolverControlFeature::options() const
{
    return m_impl->get_solver_runtime_options();
}

void SolverControlFeature::set_options(const SolverRuntimeOptions& options)
{
    UIPC_ASSERT_THROW(options.newton_max_iterations > 0,
                      "Newton maximum iterations must be positive");
    UIPC_ASSERT_THROW(options.line_search_max_iterations > 0,
                      "line-search maximum iterations must be positive");
    UIPC_ASSERT_THROW(std::isfinite(options.linear_system_tolerance_rate)
                          && options.linear_system_tolerance_rate > 0.0,
                      "linear-system tolerance rate must be finite and positive");
    m_impl->set_solver_runtime_options(options);
}

std::string_view SolverControlFeature::get_name() const
{
    return FeatureName;
}
}  // namespace uipc::core
