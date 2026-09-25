
#include "../vroum3d/vroum3d.h"
#include "core/instance.h"
#include "core/objects.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace Vroum3d;


int main(int, char*[])
{
	Display disp;
	DisplayInstance inst(disp);

	PipelineResource pr(inst);

	Pipeline pipe(pr, 
		RenderPipelineInformation(pr, "static.vert.spv", "static.frag.spv", {}, {}));

	VkHandle<VkCommandPool> pool;
	VkCommandPoolCreateInfo cpi{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		inst.graphic_queue_index()
	};

	vk_check(vkCreateCommandPool(inst.device(), &cpi, nullptr, &pool));

	bool run = true;
	std::array<CommandBuffer, Instance::c_frames_in_flight> cmd_bufs;

  for(auto& buf : cmd_bufs)
  {
    std::construct_at(&buf, inst, pool);
  }

	while(run)
	{
		if(inst.render_done() && inst.acquire_next_image())
		{
			auto& cmd_buf = cmd_bufs[inst.next_frame()];

			cmd_buf.reset();
			cmd_buf.begin_primary();
			cmd_buf.begin_rendering(inst);

			cmd_buf.bind_graphics_pipeline(inst, pipe.pipeline());
			

			cmd_buf.cmd<vkCmdDraw>(6, 1, 0, 0);

			cmd_buf.end_rendering(inst);
			cmd_buf.end();

			inst.submit_render_present(cmd_buf);
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		SDL_Event evnt;
		while(SDL_PollEvent(&evnt))
			if(evnt.type == SDL_EVENT_QUIT)
			{
				run = false;
				break;
			}
	}

	vkDeviceWaitIdle(inst.device());

	for(auto& cb : cmd_bufs) cb.destroy();
	vkDestroyCommandPool(inst.device(), pool, nullptr);

	return 0;
}
