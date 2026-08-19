#pragma once

#include <uipc/core/feature.h>

namespace uipc::core
{
struct UIPC_CORE_API SolverRuntimeOptions
{
    SizeT newton_max_iterations         = 16;
    SizeT line_search_max_iterations    = 8;
    Float linear_system_tolerance_rate  = 1.0e-3;
    bool  strict_mode                   = false;
};

class UIPC_CORE_API SolverControlFeatureOverrider
{
  public:
    virtual ~SolverControlFeatureOverrider() = default;
    virtual SolverRuntimeOptions get_solver_runtime_options() const = 0;
    virtual void set_solver_runtime_options(const SolverRuntimeOptions& options) = 0;
};

class UIPC_CORE_API SolverControlFeature final : public Feature
{
  public:
    constexpr static std::string_view FeatureName = "core/solver_control";

    explicit SolverControlFeature(S<SolverControlFeatureOverrider> overrider);

    SolverRuntimeOptions options() const;
    void set_options(const SolverRuntimeOptions& options);

  private:
    virtual std::string_view get_name() const override;
    S<SolverControlFeatureOverrider> m_impl;
};
}  // namespace uipc::core
