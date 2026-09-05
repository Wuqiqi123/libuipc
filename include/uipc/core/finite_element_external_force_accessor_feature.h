#pragma once
#include <uipc/backend/buffer_view.h>
#include <uipc/core/feature.h>

namespace uipc::core
{
class UIPC_CORE_API FiniteElementExternalForceAccessorFeatureOverrider
{
  public:
    virtual ~FiniteElementExternalForceAccessorFeatureOverrider() = default;

    virtual void do_bind_external_forces(backend::BufferView buffer_view) = 0;
    virtual void do_unbind_external_forces()                              = 0;
    virtual bool do_has_bound_external_forces() const                     = 0;
};

/**
 * @brief Binds device-resident per-vertex forces to the FEM force manager.
 *
 * The buffer contains one Vector3 per global FEM vertex and must remain alive
 * until it is unbound or the World is destroyed.
 */
class UIPC_CORE_API FiniteElementExternalForceAccessorFeature final : public Feature
{
  public:
    constexpr static std::string_view FeatureName =
        "core/finite_element_external_force_accessor";

    explicit FiniteElementExternalForceAccessorFeature(
        S<FiniteElementExternalForceAccessorFeatureOverrider> overrider);

    void bind_external_forces(backend::BufferView buffer_view) const;
    void unbind_external_forces() const;
    bool has_bound_external_forces() const;

  private:
    virtual std::string_view get_name() const override;
    S<FiniteElementExternalForceAccessorFeatureOverrider> m_impl;
};
}  // namespace uipc::core
