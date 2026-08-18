#include <uipc/core/soft_position_constraint_accessor_feature.h>

namespace uipc::core
{
SoftPositionConstraintAccessorFeature::SoftPositionConstraintAccessorFeature(
    S<SoftPositionConstraintAccessorFeatureOverrider> overrider)
    : m_impl(std::move(overrider))
{
    UIPC_ASSERT_THROW(m_impl, "SoftPositionConstraintAccessorFeatureOverrider must not be null.");
}

void SoftPositionConstraintAccessorFeature::bind_aim_positions(backend::BufferView buffer_view) const
{
    UIPC_ASSERT_THROW(buffer_view && buffer_view.backend() == "cuda",
                      "Soft position targets must use a valid CUDA buffer view.");
    UIPC_ASSERT_THROW(buffer_view.element_size() == sizeof(Vector3)
                          && buffer_view.element_stride() == sizeof(Vector3),
                      "Soft position targets must contain contiguous Vector3 values.");
    m_impl->do_bind_aim_positions(buffer_view);
}

void SoftPositionConstraintAccessorFeature::unbind_aim_positions() const
{
    m_impl->do_unbind_aim_positions();
}

bool SoftPositionConstraintAccessorFeature::has_bound_aim_positions() const
{
    return m_impl->do_has_bound_aim_positions();
}

std::string_view SoftPositionConstraintAccessorFeature::get_name() const
{
    return FeatureName;
}
}  // namespace uipc::core
