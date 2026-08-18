#include <app/app.h>
#include <uipc/uipc.h>
#include <uipc/constitution/affine_body_constitution.h>
#include <uipc/constitution/soft_transform_constraint.h>
#include <uipc/core/soft_transform_constraint_accessor_feature.h>
#include <cuda_runtime_api.h>
#include <limits>

TEST_CASE("93_solver_nonfinite_status", "[solver_failure]")
{
    using namespace uipc;
    using namespace uipc::core;
    using namespace uipc::geometry;
    using namespace uipc::constitution;

    auto output_path = AssetDir::output_path(UIPC_RELATIVE_SOURCE_FILE);
    Engine engine{"cuda", output_path};
    World  world{engine};

    auto config                 = test::Scene::default_config();
    config["gravity"]           = Vector3{0, 0, 0};
    config["contact"]["enable"] = false;
    Scene scene{config};

    AffineBodyConstitution  abd;
    SoftTransformConstraint soft_transform;
    auto object = scene.objects().create("body");

    vector<Vector4i> tetrahedra = {Vector4i{0, 1, 2, 3}};
    vector<Vector3>  vertices = {Vector3{0, 1, 0},
                                 Vector3{0, 0, 1},
                                 Vector3{-0.5, 0, -0.5},
                                 Vector3{0.5, 0, -0.5}};
    auto mesh = tetmesh(vertices, tetrahedra);
    label_surface(mesh);
    label_triangle_orient(mesh);
    abd.apply_to(mesh, 1.0e7);
    soft_transform.apply_to(mesh, Vector2{100, 100});
    view(*mesh.instances().find<IndexT>(builtin::is_constrained))[0] = 1;
    object->geometries().create(mesh);

    world.init(scene);
    REQUIRE(world.is_valid());

    auto accessor = world.features().find<SoftTransformConstraintAccessorFeature>();
    REQUIRE(accessor != nullptr);

    Matrix4x4 invalid_target = Matrix4x4::Identity();
    invalid_target(0, 3)     = std::numeric_limits<Float>::quiet_NaN();
    Matrix4x4* device_target = nullptr;
    REQUIRE(cudaMalloc(reinterpret_cast<void**>(&device_target), sizeof(Matrix4x4))
            == cudaSuccess);
    REQUIRE(cudaMemcpy(device_target,
                       &invalid_target,
                       sizeof(Matrix4x4),
                       cudaMemcpyHostToDevice)
            == cudaSuccess);

    accessor->bind_aim_transforms(backend::BufferView{
        reinterpret_cast<backend::HandleT>(device_target),
        0,
        1,
        sizeof(Matrix4x4),
        sizeof(Matrix4x4),
        "cuda"});
    world.advance();
    REQUIRE_FALSE(world.is_valid());

    accessor->unbind_aim_transforms();
    REQUIRE(cudaDeviceSynchronize() == cudaSuccess);
    REQUIRE(cudaFree(device_target) == cudaSuccess);
}
