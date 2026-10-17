#include "../vroum3d/vroum3d.h"
#include "core/instance.h"
#include "core/objects.h"
#include "core/pipeline.h"
#include "core/pipeline_info.h"
#include "core/update_descriptor_set.hpp"
#include "debug.h"
#include "raytracing/acceleration_structure.h"
#include "raytracing/instance_info.h"

#include <thread>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace Vroum3d;

int main(int, char*[])
{
  Display disp;
  DisplayInstance inst(disp, Raytracing::InstanceInfo<DisplayInstanceInfo<>>());

  PipelineResource pr(inst);

  CommandPool cmd_pool(inst, inst.compute_queue_index());

  Math::vec3 data[] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

  uint32_t indexes[] = {0, 1, 2};

  Buffer buf(inst, sizeof(data) + sizeof(indexes), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  auto* dt = buf.map();
  memcpy(dt, data, sizeof(data));
  memcpy(dt + sizeof(data), indexes, sizeof(indexes));

  buf.flush_unmap();

  Raytracing::SimpleTriangleAccelerationStructure as(inst);

  VkDeviceSize scratch_size;
  as.create_as(3, 3, scratch_size);

  {
    Buffer scratch_buffer(inst, scratch_size,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    auto addr = buf.device_address();

    as.upload_as({.deviceAddress = addr}, {.deviceAddress = addr + sizeof(data)}, cmd_pool, 3, 3, {.deviceAddress = scratch_buffer.device_address()});
  }

  DescriptorPool des_pool(inst, 32, {{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32}});

  Core::Pipeline pipe(pr, RenderPipelineInformation(pr, "ray_query.vert.spv", "ray_query.frag.spv", {}, {}));

  VkDescriptorSet ds = des_pool.allocate_descriptor_set(pipe.descriptor_set_layout(0));

  {
    using namespace UpdateDescriptorSet;
    update_descriptor_sets(inst.device(), WriteAccelerationStructure<>({ds, 0, 0}, {as.tlas()}));
  }

  CommandPool pool(inst, inst.graphic_queue_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

  bool run = true;
  CommandBufferVector cmd_bufs(inst, pool, DisplayInstance::c_frames_in_flight);

  while(run)
  {
    if(inst.render_done() && inst.acquire_next_image())
    {
      auto cmd_buf = cmd_bufs[inst.next_frame()];

      cmd_buf.reset();
      cmd_buf.begin_primary();
      cmd_buf.begin_rendering(inst);

      cmd_buf.bind_graphics_pipeline(inst, pipe.pipeline());

      cmd_buf.cmd(vkCmdBindDescriptorSets, VK_PIPELINE_BIND_POINT_COMPUTE, pipe.layout(), 0, 1, &ds, 0, nullptr);
      cmd_buf.cmd(vkCmdDraw, 3, 1, 0, 0);

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

  return 0;
}
