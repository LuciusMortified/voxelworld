export module vw.gfx:resource;

export import :meshing;
export import :mesh_pool;
export import :gpu_buffers;
export import :geometry;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :camera;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}
