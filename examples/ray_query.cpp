#include "../vroum3d/vroum3d.h"
#include "core/instance.h"
#include "core/objects.h"
#include "core/pipeline.h"
#include "core/pipeline_info.h"
#include "core/update_descriptor_set.hpp"
#include "debug.h"
#include "math/mat.hpp"
#include "raytracing/acceleration_structure.h"
#include "raytracing/instance_info.h"

#include <SDL3/SDL_events.h>
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

  Math::vec3 data[] = {{-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

  uint32_t indexes[] = {0, 1, 2, 1, 2, 3};

  Buffer buf(inst, sizeof(data) + sizeof(indexes), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  auto* dt = buf.map();
  memcpy(dt, data, sizeof(data));
  memcpy(dt + sizeof(data), indexes, sizeof(indexes));

  buf.flush_unmap();

  Raytracing::SimpleTriangleAccelerationStructure as(inst);

  VkDeviceSize scratch_size;
  as.create_as(4, 6, scratch_size);

  {
    Buffer scratch_buffer(inst, scratch_size,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 256);

    auto addr = buf.device_address();

    as.upload_as({.deviceAddress = addr}, {.deviceAddress = addr + sizeof(data)}, cmd_pool, 4, 6, scratch_buffer.buffer(),
                 {.deviceAddress = scratch_buffer.device_address()});
  }

  DescriptorPool des_pool(inst, 32, {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32}, {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32}});

  Core::Pipeline pipe(pr, RenderPipelineInformation(pr, "ray_query.vert.spv", "ray_query.frag.spv", {}, {}));

  std::array<VkDescriptorSet, Instance::c_frames_in_flight> ds;
  des_pool.allocate_descriptor_sets({pipe.descriptor_set_layout(0), pipe.descriptor_set_layout(0)}, ds);

  struct RawUniformStruct
  {
    Math::mat4 mat;
    Math::vec3 camera_pos;
  };

  struct UniformStruct : RawUniformStruct{
    char pad[sizeof(RawUniformStruct) & 63 ? 64 - (sizeof(RawUniformStruct) & 63) : 0];
  };

  Buffer uni_buf(inst, 2 * sizeof(UniformStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  UniformStruct* uni_data = reinterpret_cast<UniformStruct*>(uni_buf.map());
  
  for(std::size_t i(0); i != ds.size(); ++i)
  {
    using namespace UpdateDescriptorSet;
    update_descriptor_sets(inst.device(),
        WriteAccelerationStructure<>({ds[i], 0, 0}, {as.tlas()}),
        WriteBuffers<>({ds[i], 1, 0}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {{uni_buf.buffer(), sizeof(UniformStruct) * i, sizeof(UniformStruct)}})
    );
  }

  CommandPool pool(inst, inst.graphic_queue_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

  bool run = true;
  CommandBufferVector cmd_bufs(inst, pool, DisplayInstance::c_frames_in_flight);

  bool forw = false;
  bool backw = false;
  bool left = false;
  bool right = false;

  SDL_Event evnt;
  Math::Camera cam(Math::vec3{0.0, 0.0, -2.0});
  constexpr float c_mouse_sens = 0.01f;
  constexpr float c_timestep = 0.01f;

  disp.set_window_relative_mouse_mode(true);

  while(run)
  {
    if(inst.render_done() && inst.acquire_next_image())
    {
      auto screen_to_cam_matrix = Math::scale(Math::vec4(1.0, float(inst.h()) / float(inst.w()), 1.0, 1.0));
      screen_to_cam_matrix = cam.get_inv_matrix() * screen_to_cam_matrix;

      auto& uni = uni_data[inst.next_frame()];

      uni.mat = screen_to_cam_matrix;
      uni.camera_pos = cam.position();

      auto cmd_buf = cmd_bufs[inst.next_frame()];

      cmd_buf.reset();
      cmd_buf.begin_primary();
      cmd_buf.begin_rendering(inst);

      cmd_buf.bind_graphics_pipeline(inst, pipe.pipeline());

      cmd_buf.cmd(vkCmdBindDescriptorSets, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.layout(), 0, 1, &ds[0], 0, nullptr);
      cmd_buf.cmd(vkCmdDraw, 3, 1, 0, 0);

      cmd_buf.end_rendering(inst);
      cmd_buf.end();

      inst.submit_render_present(cmd_buf);
    }
    else
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

    while(SDL_PollEvent(&evnt))
      switch(evnt.type)
      {
      case SDL_EVENT_QUIT:
      {
        run = false;
        break;
      }
      case SDL_EVENT_MOUSE_MOTION:
        cam.add_yaw(-c_mouse_sens * evnt.motion.xrel);
        cam.add_pitch(-c_mouse_sens * evnt.motion.yrel);

        break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        if(evnt.key.repeat) break;
        bool keydown = SDL_EVENT_KEY_DOWN == evnt.type;
        switch(evnt.key.key)
        {
        case SDLK_Z:
          forw = keydown;
          break;
        case SDLK_S:
          backw = keydown;
          break;
        case SDLK_Q:
          left = keydown;
          break;
        case SDLK_D:
          right = keydown;
          break;
        }
      }

    Math::vec3 vel(0);
    if(forw) vel.z += 1.0;
    if(backw) vel.z -= 1.0;
    if(right) vel.x += 1.0;
    if(left) vel.x -= 1.0;

    cam.add_position(cam.get_inv_matrix_orient_only() * vel * c_timestep);
  }
  
  uni_buf.unmap();

  vkDeviceWaitIdle(inst.device());

  return 0;
}
