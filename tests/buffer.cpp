#include "../vroum3d/vroum3d.h"
#include <vulkan/vulkan_core.h>


using namespace Vroum3d::Core;

int main(int, char*[])
{
	Instance inst;

  Buffer bf0(inst, 1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    bf1(inst, 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    bf2(inst, 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	return 0;
}
