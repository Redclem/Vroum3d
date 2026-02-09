
#include "../vroum3d/vroum3d.h"
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace Vroum3d;

int main()
{
	Display disp;
	Instance inst(disp);

	PipelineResource pr(inst);

	Pipeline pipe(pr, BasicPipelineInformation(pr, "static.vert.spv", "static.frag.spv", {}, {}));

	VkHandle<VkCommandPool> pool;
	VkCommandPoolCreateInfo cpi{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		0,
		inst.graphic_queue_index()
	};

	vk_check(vkCreateCommandPool(inst.device(), &cpi, nullptr, &pool));

	return 0;
}
