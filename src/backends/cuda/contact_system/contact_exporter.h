#pragma once
#include <sim_system.h>
#include <uipc/core/contact_system_feature.h>
#include <muda/buffer/buffer_view.h>
#include <muda/ext/linear_system/doublet_vector_view.h>

namespace uipc::backend::cuda
{
class ContactExporter : public SimSystem
{
  public:
    using SimSystem::SimSystem;

    std::string_view prim_type() const noexcept;

    class BuildInfo
    {
      public:
    };

  protected:
    virtual void             do_build(BuildInfo& info)      = 0;
    virtual std::string_view get_prim_type() const noexcept = 0;

    virtual void get_contact_energy(std::string_view    prim_type,
                                    geometry::Geometry& energy_geo) = 0;

    virtual void get_contact_gradient(std::string_view    prim_type,
                                      geometry::Geometry& vert_grad) = 0;

    virtual void get_contact_hessian(std::string_view    prim_type,
                                     geometry::Geometry& vert_hess) = 0;

    virtual core::ContactGradientDeviceView get_contact_gradient_device_view(std::string_view prim_type)
    {
        return {};
    }

    static core::ContactGradientDeviceView make_contact_gradient_device_view(
        muda::CDoubletVectorView<Float, 3> gradient)
    {
        auto indices = gradient.indices();
        auto values  = gradient.values();
        return {backend::BufferView{static_cast<backend::HandleT>(reinterpret_cast<std::uintptr_t>(
                                        const_cast<IndexT*>(indices.data()))),
                                    0,
                                    indices.size(),
                                    sizeof(IndexT),
                                    sizeof(IndexT),
                                    "cuda"},
                backend::BufferView{static_cast<backend::HandleT>(reinterpret_cast<std::uintptr_t>(
                                        const_cast<Vector3*>(values.data()))),
                                    0,
                                    values.size(),
                                    sizeof(Vector3),
                                    sizeof(Vector3),
                                    "cuda"},
                true};
    }

  private:
    virtual void do_build() override final;

    friend class ContactExporterManager;
    void contact_energy(std::string_view prim_type, geometry::Geometry& energy_geo);
    void contact_gradient(std::string_view prim_type, geometry::Geometry& vert_grad);
    void contact_hessian(std::string_view prim_type, geometry::Geometry& vert_hess);
    core::ContactGradientDeviceView contact_gradient_device_view(std::string_view prim_type);
};
}  // namespace uipc::backend::cuda
