#include "../vroum3d/vroum3d.h"
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

int VROUM3D_MAIN()
{
	Display disp;
	Instance inst(disp);

	PipelineResource pr(inst);

	Pipeline pipe(pr, RenderPipelineInformation(pr, "simple.vert.spv", "simple.frag.spv", {{12}}, {{0, 0}}));

	VkDescriptorSetLayout dsl_uni;
	{
		PipelineResource::DescriptorSetDescription des;
		des.add_binding(0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT
		       | VK_SHADER_STAGE_FRAGMENT_BIT);
		
		dsl_uni = pr.get_descriptor_set_layout(std::move(des));
	}

	(void)dsl_uni;

	return 0;
}
