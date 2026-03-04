#ifndef VROUM3D_CORE_ALLOCATOR_H_INCLUDED
#define VROUM3D_CORE_ALLOCATOR_H_INCLUDED

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{


class Allocator
{
	VkDevice m_device;
	VkPhysicalDevice m_pdev;

public:
	friend class Instance;

	VkDevice device() const{return m_device;}

	VkPhysicalDevice pdev() const {return m_pdev;}
private:
	Allocator();

	void init(VkDevice dev, VkPhysicalDevice pdev);

	void destroy();
};

}

#endif
