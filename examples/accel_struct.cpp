#include "../vroum3d/vroum3d.h"
#include "raytracing/acceleration_structure.h"
#include "raytracing/instance_info.h"
#include <cstring>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace Vroum3d;

int main(int, char*[])
{
	Instance inst(Raytracing::InstanceInfo<>{});

  Math::vec3 data[] = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f}
  };

  uint32_t indexes[] = {
    0, 1, 2
  };

  Buffer buf(inst, sizeof(data) + sizeof(indexes), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  auto* dt = buf.map();
  memcpy(dt, data, sizeof(data));
  memcpy(dt + sizeof(data), indexes, sizeof(indexes));

  buf.unmap();

  Raytracing::SimpleTriangleAccelerationStructure as(inst, buf, 3, 3);

	return 0;
}
