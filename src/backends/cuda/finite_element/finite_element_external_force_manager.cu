#include <finite_element/finite_element_external_force_manager.h>
#include <finite_element/finite_element_method.h>
#include <finite_element/finite_element_external_force_reporter.h>
#include <uipc/core/finite_element_external_force_accessor_feature.h>

namespace uipc::backend::cuda
{
class FEMExternalForceAccessorFeatureOverrider final
    : public core::FiniteElementExternalForceAccessorFeatureOverrider
{
  public:
    explicit FEMExternalForceAccessorFeatureOverrider(
        FEMExternalForceManager& manager)
        : m_manager(manager)
    {
    }

    void do_bind_external_forces(backend::BufferView buffer_view) override
    {
        m_manager.bind_external_forces(buffer_view);
    }

    void do_unbind_external_forces() override
    {
        m_manager.unbind_external_forces();
    }

    bool do_has_bound_external_forces() const override
    {
        return m_manager.has_bound_external_forces();
    }

  private:
    FEMExternalForceManager& m_manager;
};

REGISTER_SIM_SYSTEM(FEMExternalForceManager);

void FEMExternalForceManager::do_build(BuildInfo& info)
{
    m_impl.finite_element_method = &require<FiniteElementMethod>();
    auto overrider =
        std::make_shared<FEMExternalForceAccessorFeatureOverrider>(*this);
    features().insert(
        std::make_shared<core::FiniteElementExternalForceAccessorFeature>(
            overrider));
}

void FEMExternalForceManager::register_reporter(FiniteElementExternalForceReporter* reporter)
{
    check_state(SimEngineState::BuildSystems, "register_reporter");
    m_impl.m_reporters.register_sim_system(*reporter);
}

void FEMExternalForceManager::Impl::clear()
{
    auto external_forces =
        finite_element_method->m_impl.vertex_external_forces.view();

    using namespace muda;
    ParallelFor()
        .file_line(__FILE__, __LINE__)
        .apply(external_forces.size(),
               [forces = external_forces.viewer().name("forces")] __device__(int i) mutable
               { forces(i).setZero(); });
}

void FEMExternalForceManager::Impl::step()
{
    ExternalForceInfo info{this};
    for(auto reporter : m_reporters.view())
    {
        reporter->step(info);
    }

    using namespace muda;

    auto& fem = finite_element_method->m_impl;

    auto force_accs = fem.vertex_external_force_accs.view();
    auto forces     = fem.vertex_external_forces.view();
    auto masses     = finite_element_method->masses();

    SizeT vertex_count = forces.size();

    if(external_force_input)
    {
        auto* input_ptr = reinterpret_cast<const Vector3*>(
                              external_force_input.handle())
                          + external_force_input.offset();
        muda::CBufferView<Vector3> input{input_ptr, vertex_count};
        ParallelFor()
            .file_line(__FILE__, __LINE__)
            .apply(vertex_count,
                   [forces = forces.viewer().name("forces"),
                    input  = input.viewer().name("external_force_input")] __device__(
                       int i) mutable { forces(i) += input(i); });
    }

    ParallelFor()
        .file_line(__FILE__, __LINE__)
        .apply(vertex_count,
               [forces     = forces.cviewer().name("forces"),
                force_accs = force_accs.viewer().name("force_accs"),
                masses     = masses.cviewer().name("masses")] __device__(int i)
               {
                   const Vector3& F = forces(i);
                   Float          m = masses(i);

                   // a = F / m (avoid division by zero for massless vertices)
                   if(m > 0.0)
                       force_accs(i) = F / m;
                   else
                       force_accs(i).setZero();
               });
}

void FEMExternalForceManager::do_init()
{
    for(auto reporter : m_impl.m_reporters.view())
    {
        reporter->init();
    }
}

void FEMExternalForceManager::do_clear()
{
    m_impl.clear();
}

void FEMExternalForceManager::do_step()
{
    m_impl.step();
}

muda::BufferView<Vector3> FEMExternalForceManager::ExternalForceInfo::external_forces() noexcept
{
    return m_impl->finite_element_method->m_impl.vertex_external_forces.view();
}

void FEMExternalForceManager::bind_external_forces(
    backend::BufferView buffer_view)
{
    const SizeT required_count =
        m_impl.finite_element_method->m_impl.vertex_external_forces.size();
    UIPC_ASSERT_THROW(
        buffer_view.size() >= required_count,
        "FEM external-force buffer contains {} vertices, but {} are required.",
        buffer_view.size(),
        required_count);
    m_impl.external_force_input = buffer_view;
}

void FEMExternalForceManager::unbind_external_forces()
{
    m_impl.external_force_input = {};
}

bool FEMExternalForceManager::has_bound_external_forces() const
{
    return m_impl.external_force_input.operator bool();
}
}  // namespace uipc::backend::cuda
