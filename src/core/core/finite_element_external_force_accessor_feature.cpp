#include <uipc/core/finite_element_external_force_accessor_feature.h>
#include <uipc/common/type_define.h>

namespace uipc::core
{
FiniteElementExternalForceAccessorFeature::FiniteElementExternalForceAccessorFeature(
    S<FiniteElementExternalForceAccessorFeatureOverrider> overrider)
    : m_impl(std::move(overrider))
{
    UIPC_ASSERT_THROW(
        m_impl,
        "FiniteElementExternalForceAccessorFeatureOverrider must not be null.");
}

void FiniteElementExternalForceAccessorFeature::bind_external_forces(
    backend::BufferView buffer_view) const
{
    UIPC_ASSERT_THROW(buffer_view && buffer_view.backend() == "cuda",
                      "FEM external forces must use a valid CUDA buffer view.");
    UIPC_ASSERT_THROW(
        buffer_view.element_size() == sizeof(Vector3)
            && buffer_view.element_stride() == sizeof(Vector3),
        "FEM external forces must contain contiguous Vector3 values.");
    m_impl->do_bind_external_forces(buffer_view);
}

void FiniteElementExternalForceAccessorFeature::unbind_external_forces() const
{
    m_impl->do_unbind_external_forces();
}

bool FiniteElementExternalForceAccessorFeature::has_bound_external_forces() const
{
    return m_impl->do_has_bound_external_forces();
}

std::string_view FiniteElementExternalForceAccessorFeature::get_name() const
{
    return FeatureName;
}
}  // namespace uipc::core
