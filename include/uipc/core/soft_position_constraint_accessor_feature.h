#pragma once
#include <uipc/core/feature.h>
#include <uipc/backend/buffer_view.h>

namespace uipc::core
{
class UIPC_CORE_API SoftPositionConstraintAccessorFeatureOverrider
{
  public:
    virtual ~SoftPositionConstraintAccessorFeatureOverrider() = default;

    virtual void do_bind_aim_positions(backend::BufferView buffer_view) = 0;
    virtual void do_unbind_aim_positions()                              = 0;
    virtual bool do_has_bound_aim_positions() const                     = 0;
};

/**
 * @brief Binds device-resident FEM targets to SoftPositionConstraint.
 *
 * The buffer contains one Vector3 per global FEM vertex index and must remain
 * alive until it is unbound or the World is destroyed.
 */
class UIPC_CORE_API SoftPositionConstraintAccessorFeature final : public Feature
{
  public:
    constexpr static std::string_view FeatureName = "core/soft_position_constraint_accessor";

    explicit SoftPositionConstraintAccessorFeature(S<SoftPositionConstraintAccessorFeatureOverrider> overrider);

    void bind_aim_positions(backend::BufferView buffer_view) const;
    void unbind_aim_positions() const;
    bool has_bound_aim_positions() const;

  private:
    virtual std::string_view                          get_name() const override;
    S<SoftPositionConstraintAccessorFeatureOverrider> m_impl;
};
}  // namespace uipc::core
