#ifndef VROUM3D_CORE_SUBMISSION_HPP_INCLUDED
#define VROUM3D_CORE_SUBMISSION_HPP_INCLUDED

#include "../volk_wrapper.h"

#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

class SemaphoreSubmitInfo : public VkSemaphoreSubmitInfo {
public:
  SemaphoreSubmitInfo(VkSemaphore sem, uint64_t val, VkPipelineStageFlags2 stages) :
    VkSemaphoreSubmitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, nullptr, sem, val, stages, 0}
  {}
};

class CommandBufferSubmitInfo : public VkCommandBufferSubmitInfo {
public:
  CommandBufferSubmitInfo(VkCommandBuffer cmdb) : VkCommandBufferSubmitInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, nullptr, cmdb, 0} {}
};

template<typename WaitSemInfos = std::initializer_list<SemaphoreSubmitInfo>, typename CommandBufferInfos = std::initializer_list<CommandBufferSubmitInfo>,
         typename SignalSemInfos = std::initializer_list<SemaphoreSubmitInfo>>
class SubmitInfo {
public:
  SubmitInfo(WaitSemInfos &&wsi, CommandBufferInfos &&cmdi, SignalSemInfos &&ssi) :
    m_wait_sem_infos(std::forward<WaitSemInfos>(wsi)), m_command_buffer_infos(std::forward<CommandBufferInfos>(cmdi)),
    m_signal_sem_infos(std::forward<SignalSemInfos>(ssi))
  {}

  VkSubmitInfo2 submit_info() const
  {
    return {VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            nullptr,
            0,
            uint32_t(std::size(m_wait_sem_infos)),
            std::data(m_wait_sem_infos),
            uint32_t(std::size(m_command_buffer_infos)),
            std::data(m_command_buffer_infos),
            uint32_t(std::size(m_signal_sem_infos)),
            std::data(m_signal_sem_infos)};
  }

private:
  std::remove_cvref_t<WaitSemInfos> m_wait_sem_infos;
  std::remove_cvref_t<CommandBufferInfos> m_command_buffer_infos;
  std::remove_cvref_t<SignalSemInfos> m_signal_sem_infos;
};



} // namespace Vroum3d::Core

#endif
