#include <uipc/core/soft_transform_constraint_accessor_feature.h>

namespace uipc::core
{
SoftTransformConstraintAccessorFeature::SoftTransformConstraintAccessorFeature(
    S<SoftTransformConstraintAccessorFeatureOverrider> overrider)
    : m_impl(std::move(overrider))
{
    UIPC_ASSERT_THROW(m_impl, "SoftTransformConstraintAccessorFeatureOverrider must not be null.");
}

void SoftTransformConstraintAccessorFeature::bind_aim_transforms(backend::BufferView buffer_view) const
{
    UIPC_ASSERT_THROW(buffer_view && buffer_view.backend() == "cuda",
                      "Soft transform targets must use a valid CUDA buffer view.");
    UIPC_ASSERT_THROW(buffer_view.element_size() == sizeof(Matrix4x4)
                          && buffer_view.element_stride() == sizeof(Matrix4x4),
                      "Soft transform targets must contain contiguous Matrix4x4 values.");
    m_impl->do_bind_aim_transforms(buffer_view);
}

void SoftTransformConstraintAccessorFeature::unbind_aim_transforms() const
{
    m_impl->do_unbind_aim_transforms();
}

bool SoftTransformConstraintAccessorFeature::has_bound_aim_transforms() const
{
    return m_impl->do_has_bound_aim_transforms();
}

std::string_view SoftTransformConstraintAccessorFeature::get_name() const
{
    return FeatureName;
}
}  // namespace uipc::core
