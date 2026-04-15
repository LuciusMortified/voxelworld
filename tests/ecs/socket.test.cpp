#include <catch2/catch_test_macros.hpp>

#include <vw/gfx/world/components/socket_component.h>
#include <vw/gfx/world/components/hierarchy_component.h>
#include <vw/gfx/world/components/transform_component.h>
#include <vw/gfx/world/components/spatial_component.h>
#include <vw/gfx/world/systems/socket_system.h>
#include <vw/gfx/world/systems/hierarchy_system.h>
#include <vw/gfx/world/systems/transform_system.h>
#include <vw/gfx/world/systems/spatial_system.h>
#include <vw/gfx/world/world_def.h>

using namespace vw;
using namespace vw::gfx;

TEST_CASE("socket_component default construction", "[socket]") {
    socket_component comp;
    REQUIRE(comp.get_sockets().empty());
}

TEST_CASE("socket_component construction with sockets", "[socket]") {
    socket_component comp({
        socket_point{"hand"},
        socket_point{"head"},
    });
    REQUIRE(comp.get_sockets().size() == 2);
}

TEST_CASE("socket_component find returns nullptr for empty", "[socket]") {
    socket_component comp;
    REQUIRE(comp.find("hand") == nullptr);
}

TEST_CASE("socket_component find returns socket_point", "[socket]") {
    socket_component comp({
        socket_point{"hand"},
        socket_point{"head"},
    });

    auto* hand = comp.find("hand");
    REQUIRE(hand != nullptr);
    REQUIRE(hand->name == "hand");

    auto* head = comp.find("head");
    REQUIRE(head != nullptr);
    REQUIRE(head->name == "head");

    REQUIRE(comp.find("foot") == nullptr);
}

TEST_CASE("socket_component is_occupied with empty socket", "[socket]") {
    socket_component comp({
        socket_point{"hand"},
    });

    REQUIRE_FALSE(comp.is_occupied("hand"));
    REQUIRE_FALSE(comp.is_occupied("nonexistent"));
}

TEST_CASE("socket_component is_occupied with attached entity", "[socket]") {
    entity child{0, 0};
    socket_component comp({
        socket_point{"hand", {}, {}, {1, 1, 1}, child},
    });

    REQUIRE(comp.is_occupied("hand"));
}

TEST_CASE("socket_component get_sockets returns all sockets", "[socket]") {
    socket_component comp({
        socket_point{"a"},
        socket_point{"b"},
        socket_point{"c"},
    });

    REQUIRE(comp.get_sockets().size() == 3);
    REQUIRE(comp.get_sockets()[0].name == "a");
    REQUIRE(comp.get_sockets()[1].name == "b");
    REQUIRE(comp.get_sockets()[2].name == "c");
}

TEST_CASE("socket_point default attached is invalid", "[socket]") {
    socket_point sp{"test"};
    REQUIRE_FALSE(sp.attached.is_valid());
}

using test_def = world_def<hierarchy_system, transform_system, spatial_system, socket_system>;

struct socket_test_fixture {
    test_def::registry_type reg;
    world_context<test_def> ctx{reg};
    test_def::systems_tuple systems_{
        hierarchy_system<test_def>{ctx},
        transform_system<test_def>{ctx},
        spatial_system<test_def>{ctx},
        socket_system<test_def>{ctx}};

    socket_system<test_def>& socket_sys = std::get<socket_system<test_def>>(systems_);
    hierarchy_system<test_def>& hierarchy_sys = std::get<hierarchy_system<test_def>>(systems_);
    transform_system<test_def>& transform_sys = std::get<transform_system<test_def>>(systems_);

    socket_test_fixture() {
        ctx.set_systems_ptr_(&systems_);
    }

    auto create_entity() -> entity {
        auto ent = reg.create();
        reg.add(ent, hierarchy_component{});
        reg.add(ent, transform_component{});
        return ent;
    }

    auto create_entity_with_socket(std::vector<socket_point> sockets) -> entity {
        auto ent = create_entity();
        reg.add(ent, socket_component{std::move(sockets)});
        return ent;
    }
};

TEST_CASE("socket_system attach sets parent and transform", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
    });
    auto child = f.create_entity();

    f.socket_sys.modify(parent).attach("hand", child);

    auto& socket_comp = f.reg.get<socket_component>(parent);
    auto* hand = socket_comp.find("hand");
    REQUIRE(hand != nullptr);
    REQUIRE(hand->attached == child);

    auto& child_hierarchy = f.reg.get<hierarchy_component>(child);
    REQUIRE(child_hierarchy.has_parent());
    REQUIRE(child_hierarchy.get_parent() == parent);
}

TEST_CASE("socket_system attach to occupied socket does nothing", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
    });
    auto child1 = f.create_entity();
    auto child2 = f.create_entity();

    f.socket_sys.modify(parent).attach("hand", child1);
    f.socket_sys.modify(parent).attach("hand", child2);

    auto& socket_comp = f.reg.get<socket_component>(parent);
    REQUIRE(socket_comp.find("hand")->attached == child1);
}

TEST_CASE("socket_system attach to nonexistent socket does nothing", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
    });
    auto child = f.create_entity();

    f.socket_sys.modify(parent).attach("foot", child);

    auto& child_hierarchy = f.reg.get<hierarchy_component>(child);
    REQUIRE_FALSE(child_hierarchy.has_parent());
}

TEST_CASE("socket_system detach removes parent relationship", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
    });
    auto child = f.create_entity();

    f.socket_sys.modify(parent).attach("hand", child);
    f.socket_sys.modify(parent).detach("hand");

    auto& socket_comp = f.reg.get<socket_component>(parent);
    REQUIRE_FALSE(socket_comp.is_occupied("hand"));

    auto& child_hierarchy = f.reg.get<hierarchy_component>(child);
    REQUIRE_FALSE(child_hierarchy.has_parent());
}

TEST_CASE("socket_system detach from empty socket does nothing", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
    });

    REQUIRE_NOTHROW(f.socket_sys.modify(parent).detach("hand"));
}

TEST_CASE("socket_system add_socket adds new socket point", "[socket_system]") {
    socket_test_fixture f;

    auto ent = f.create_entity();
    f.reg.add(ent, socket_component{});

    f.socket_sys.modify(ent).add_socket("hand");

    auto& socket_comp = f.reg.get<socket_component>(ent);
    REQUIRE(socket_comp.get_sockets().size() == 1);
    REQUIRE(socket_comp.find("hand") != nullptr);
}

TEST_CASE("socket_system remove_socket removes socket point", "[socket_system]") {
    socket_test_fixture f;

    auto ent = f.create_entity_with_socket({
        socket_point{"hand"},
        socket_point{"head"},
    });

    f.socket_sys.modify(ent).remove_socket("hand");

    auto& socket_comp = f.reg.get<socket_component>(ent);
    REQUIRE(socket_comp.get_sockets().size() == 1);
    REQUIRE(socket_comp.find("hand") == nullptr);
    REQUIRE(socket_comp.find("head") != nullptr);
}

TEST_CASE("socket_system remove_socket detaches child first", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
    });
    auto child = f.create_entity();

    f.socket_sys.modify(parent).attach("hand", child);
    f.socket_sys.modify(parent).remove_socket("hand");

    auto& socket_comp = f.reg.get<socket_component>(parent);
    REQUIRE(socket_comp.get_sockets().empty());

    auto& child_hierarchy = f.reg.get<hierarchy_component>(child);
    REQUIRE_FALSE(child_hierarchy.has_parent());
}

TEST_CASE("socket_system cleanup detaches all children", "[socket_system]") {
    socket_test_fixture f;

    auto parent = f.create_entity_with_socket({
        socket_point{"hand"},
        socket_point{"head"},
    });
    auto child1 = f.create_entity();
    auto child2 = f.create_entity();

    f.socket_sys.modify(parent).attach("hand", child1);
    f.socket_sys.modify(parent).attach("head", child2);

    f.socket_sys.cleanup(parent);

    auto& child1_hierarchy = f.reg.get<hierarchy_component>(child1);
    auto& child2_hierarchy = f.reg.get<hierarchy_component>(child2);
    REQUIRE_FALSE(child1_hierarchy.has_parent());
    REQUIRE_FALSE(child2_hierarchy.has_parent());
}

TEST_CASE("socket_system chaining multiple operations", "[socket_system]") {
    socket_test_fixture f;

    auto ent = f.create_entity();
    f.reg.add(ent, socket_component{});
    auto child = f.create_entity();

    f.socket_sys.modify(ent)
        .add_socket("hand")
        .attach("hand", child);

    auto& socket_comp = f.reg.get<socket_component>(ent);
    REQUIRE(socket_comp.find("hand")->attached == child);
}
