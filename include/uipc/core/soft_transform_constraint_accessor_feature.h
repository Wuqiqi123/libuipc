#pragma once
#include <uipc/core/feature.h>
#include <uipc/backend/buffer_view.h>

namespace uipc::core
{
class UIPC_CORE_API SoftTransformConstraintAccessorFeatureOverrider
{
  public:
    virtual ~SoftTransformConstraintAccessorFeatureOverrider() = default;

    virtual void do_bind_aim_transforms(backend::BufferView buffer_view) = 0;
    virtual void do_unbind_aim_transforms()                              = 0;
    virtual bool do_has_bound_aim_transforms() const                     = 0;
};

/**
 * @brief Binds device-resident affine-body targets to SoftTransformConstraint.
 *
 * The buffer contains one Matrix4x4 per global affine-body index and must
 * remain alive until it is unbound or the World is destroyed.
 */
class UIPC_CORE_API SoftTransformConstraintAccessorFeature final : public Feature
{
  public:
    constexpr static std::string_view FeatureName = "core/soft_transform_constraint_accessor";

    explicit SoftTransformConstraintAccessorFeature(S<SoftTransformConstraintAccessorFeatureOverrider> overrider);

    void bind_aim_transforms(backend::BufferView buffer_view) const;
    void unbind_aim_transforms() const;
    bool has_bound_aim_transforms() const;

  private:
    virtual std::string_view get_name() const override;
    S<SoftTransformConstraintAccessorFeatureOverrider> m_impl;
};
}  // namespace uipc::core
