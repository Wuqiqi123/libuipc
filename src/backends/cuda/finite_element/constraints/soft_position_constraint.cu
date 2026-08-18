#include <finite_element/finite_element_constraint.h>
#include <uipc/builtin/attribute_name.h>
#include <uipc/core/soft_position_constraint_accessor_feature.h>

namespace uipc::backend::cuda
{
class SoftPositionConstraint;

class SoftPositionConstraintAccessorFeatureOverrider final
    : public core::SoftPositionConstraintAccessorFeatureOverrider
{
  public:
    explicit SoftPositionConstraintAccessorFeatureOverrider(SoftPositionConstraint& constraint)
        : m_constraint(constraint)
    {
    }

    void do_bind_aim_positions(backend::BufferView buffer_view) override;
    void do_unbind_aim_positions() override;
    bool do_has_bound_aim_positions() const override;

  private:
    SoftPositionConstraint& m_constraint;
};

class SoftPositionConstraint final : public FiniteElementConstraint
{
    static constexpr U64 SoftPositionConstraintUID = 14ull;

  public:
    using FiniteElementConstraint::FiniteElementConstraint;

    vector<IndexT>  h_constrained_vertices;
    vector<Vector3> h_aim_positions;
    vector<Float>   h_strength_ratios;

    muda::DeviceBuffer<IndexT>  constrained_vertices;
    muda::DeviceBuffer<Vector3> aim_positions;
    muda::DeviceBuffer<Float>   strength_ratios;
    backend::BufferView         external_aim_positions;

    void do_build(BuildInfo& info) override
    {
        auto overrider =
            std::make_shared<SoftPositionConstraintAccessorFeatureOverrider>(*this);
        features().insert(
            std::make_shared<core::SoftPositionConstraintAccessorFeature>(overrider));
    }

    U64 get_uid() const noexcept override { return SoftPositionConstraintUID; }

    void do_init(FiniteElementAnimator::FilteredInfo& info) override
    {
        do_step(info);  // do the same thing as do_step
    }

    void do_step(FiniteElementAnimator::FilteredInfo& info) override
    {
        using ForEachInfo = FiniteElementMethod::ForEachInfo;

        auto geo_slots = world().scene().geometries();

        // clear
        h_constrained_vertices.clear();
        h_aim_positions.clear();
        h_strength_ratios.clear();

        IndexT current_vertex_offset = 0;
        info.for_each(
            geo_slots,
            [&](geometry::SimplicialComplex& sc)
            {
                auto vertex_offset =
                    sc.meta().find<IndexT>(builtin::backend_fem_vertex_offset);
                current_vertex_offset = vertex_offset->view().front();

                auto is_constrained = sc.vertices().find<IndexT>(builtin::is_constrained);
                auto aim_pos = sc.vertices().find<Vector3>(builtin::aim_position);
                auto strength_ratio = sc.vertices().find<Float>("strength_ratio");

                return zip(is_constrained->view(),
                           aim_pos->view(),
                           strength_ratio->view());
            },
            [&](const ForEachInfo& I, auto&& values)
            {
                auto vI = I.local_index() + current_vertex_offset;

                auto&& [is_constrained, aim_pos, strength] = values;

                if(is_constrained)
                {
                    h_constrained_vertices.push_back(vI);
                    if(!external_aim_positions)
                        h_aim_positions.push_back(aim_pos);
                    h_strength_ratios.push_back(strength);
                }
            });

        constrained_vertices.resize(h_constrained_vertices.size());
        constrained_vertices.view().copy_from(h_constrained_vertices.data());

        if(external_aim_positions && !h_constrained_vertices.empty())
        {
            const SizeT required_count =
                static_cast<SizeT>(*std::ranges::max_element(h_constrained_vertices) + 1);
            UIPC_ASSERT_THROW(external_aim_positions.size() >= required_count,
                              "Soft position target buffer contains {} vertices, but constrained vertex {} is required.",
                              external_aim_positions.size(),
                              required_count - 1);
        }
        else if(!external_aim_positions)
        {
            aim_positions.resize(h_aim_positions.size());
            aim_positions.view().copy_from(h_aim_positions.data());
        }

        strength_ratios.resize(h_strength_ratios.size());
        strength_ratios.view().copy_from(h_strength_ratios.data());
    }

    void do_report_extent(FiniteElementAnimator::ReportExtentInfo& info) override
    {
        info.energy_count(h_constrained_vertices.size());
        info.gradient_count(h_constrained_vertices.size());

        if(info.gradient_only())
            return;

        info.hessian_count(h_constrained_vertices.size());
    }

    void do_compute_energy(FiniteElementAnimator::ComputeEnergyInfo& info) override
    {
        using namespace muda;

        auto* external_ptr =
            external_aim_positions ?
                reinterpret_cast<const Vector3*>(external_aim_positions.handle())
                    + external_aim_positions.offset() :
                nullptr;
        CBufferView<Vector3> external_aims{
            external_ptr, external_aim_positions ? external_aim_positions.size() : 0};
        const bool use_external_aims = external_aim_positions.operator bool();

        ParallelFor()
            .file_line(__FILE__, __LINE__)
            .apply(constrained_vertices.size(),
                   [substep_ratio = info.substep_ratio(),
                    indices = constrained_vertices.viewer().name("indices"),
                    xs      = info.xs().viewer().name("xs"),
                    x_prevs = info.x_prevs().viewer().name("x_prevs"),
                    aim_positions = aim_positions.viewer().name("aim_positions"),
                    external_aims = external_aims.cviewer().name("external_aims"),
                    use_external_aims,
                    strength_ratio = strength_ratios.viewer().name("strength_ratio"),
                    masses   = info.masses().viewer().name("masses"),
                    energies = info.energies().viewer().name("energies"),
                    is_fixed = info.is_fixed().viewer().name("is_fixed")] __device__(int I)
                   {
                       auto  i = indices(I);
                       auto& E = energies(I);

                       if(is_fixed(i))
                       {
                           E = 0.0;
                       }
                       else
                       {
                           Vector3 x      = xs(i);
                           Vector3 x_prev = x_prevs(i);
                           Vector3 target = use_external_aims ? external_aims(i) :
                                                                aim_positions(I);
                           Vector3 aim_x = lerp(x_prev, target, substep_ratio);
                           Float   m     = masses(i);
                           Float   s     = strength_ratio(I);
                           Vector3 dx    = x - aim_x;

                           E = 0.5 * s * m * dx.dot(dx);
                       }
                   });
    }

    void do_compute_gradient_hessian(FiniteElementAnimator::ComputeGradientHessianInfo& info) override
    {
        using namespace muda;

        auto* external_ptr =
            external_aim_positions ?
                reinterpret_cast<const Vector3*>(external_aim_positions.handle())
                    + external_aim_positions.offset() :
                nullptr;
        CBufferView<Vector3> external_aims{
            external_ptr, external_aim_positions ? external_aim_positions.size() : 0};
        const bool use_external_aims = external_aim_positions.operator bool();

        ParallelFor()
            .file_line(__FILE__, __LINE__)
            .apply(constrained_vertices.size(),
                   [substep_ratio = info.substep_ratio(),
                    indices = constrained_vertices.viewer().name("indices"),
                    xs      = info.xs().viewer().name("xs"),
                    x_prevs = info.x_prevs().viewer().name("x_prevs"),
                    aim_positions = aim_positions.viewer().name("aim_positions"),
                    external_aims = external_aims.cviewer().name("external_aims"),
                    use_external_aims,
                    strength_ratio = strength_ratios.viewer().name("strength_ratio"),
                    masses    = info.masses().viewer().name("masses"),
                    gradients = info.gradients().viewer().name("gradients"),
                    hessians  = info.hessians().viewer().name("hessians"),
                    is_fixed  = info.is_fixed().viewer().name("is_fixed"),
                    gradient_only = info.gradient_only()] __device__(int I) mutable
                   {
                       auto    i = indices(I);
                       Vector3 G;
                       Float   m = 0.0;
                       Float   s = 0.0;
                       if(is_fixed(i))
                       {
                           G = Vector3::Zero();
                       }
                       else
                       {
                           Vector3 x      = xs(i);
                           Vector3 x_prev = x_prevs(i);
                           Vector3 target = use_external_aims ? external_aims(i) :
                                                                aim_positions(I);
                           Vector3 aim_x = lerp(x_prev, target, substep_ratio);
                           m             = masses(i);
                           s             = strength_ratio(I);
                           Vector3 dx    = x - aim_x;

                           G = s * m * dx;
                       }

                       gradients(I).write(i, G);

                       if(gradient_only)
                           return;

                       Matrix3x3 H = s * m * Matrix3x3::Identity();
                       if(is_fixed(i))
                           H = Matrix3x3::Zero();
                       hessians(I).write(i, i, H);
                   });
    }

    void bind_aim_positions(backend::BufferView buffer_view)
    {
        const auto required_count =
            h_constrained_vertices.empty() ?
                SizeT{0} :
                static_cast<SizeT>(*std::ranges::max_element(h_constrained_vertices) + 1);
        UIPC_ASSERT_THROW(buffer_view.size() >= required_count,
                          "Soft position target buffer contains {} vertices, but constrained vertex {} is required.",
                          buffer_view.size(),
                          required_count == 0 ? 0 : required_count - 1);
        external_aim_positions = buffer_view;
    }

    void unbind_aim_positions() { external_aim_positions = {}; }

    bool has_bound_aim_positions() const
    {
        return external_aim_positions.operator bool();
    }
};

void SoftPositionConstraintAccessorFeatureOverrider::do_bind_aim_positions(backend::BufferView buffer_view)
{
    m_constraint.bind_aim_positions(buffer_view);
}

void SoftPositionConstraintAccessorFeatureOverrider::do_unbind_aim_positions()
{
    m_constraint.unbind_aim_positions();
}

bool SoftPositionConstraintAccessorFeatureOverrider::do_has_bound_aim_positions() const
{
    return m_constraint.has_bound_aim_positions();
}

REGISTER_SIM_SYSTEM(SoftPositionConstraint);
}  // namespace uipc::backend::cuda
