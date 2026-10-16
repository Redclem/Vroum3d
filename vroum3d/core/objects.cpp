#include "objects.h"
#include "instance.h"
#include "vkutil.h"
#include "../volk_wrapper.h"

using namespace Vroum3d::Core;

void Buffer::create_buffer(VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags memprops)
{
	VkBufferCreateInfo bi{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		bs,
		use,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	vk_check(vkCreateBuffer(m_dev, &bi, nullptr, &m_buffer));
	
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(m_dev, m_buffer, &mr);

  m_mem = m_alloc->allocate(m_alloc->find_mem_index(mr, memprops), mr.size, mr.alignment);

	vk_check(vkBindBufferMemory(m_dev, m_buffer, m_mem.memory(), m_mem.offset()));
}
