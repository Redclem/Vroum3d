#include "../vroum3d/vroum3d.h"
#include <vulkan/vulkan_core.h>
#include <random>


using namespace Vroum3d::Core;

constexpr VkDeviceSize buffer_size = 1024;

int main(int, char*[])
{
	Instance inst;

  Buffer bf0(inst, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    bf1(inst, buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    bf2(inst, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	std::ranlux48 rng;

	std::array<char, buffer_size> data;
	for(auto& elem : data) elem = rng();

	void* ptr = bf0.map();
	std::memcpy(ptr, data.data(), buffer_size);

	bf0.flush_unmap();

	ptr = bf2.map();
	std::memcpy(ptr, data.data(), buffer_size);

	bf2.flush_unmap();

	ptr = bf0.map();
	if(memcmp(ptr, data.data(), buffer_size)) return 1;
	bf0.unmap();

	ptr = bf2.map();
	if(memcmp(ptr, data.data(), buffer_size)) return 1;
	bf2.unmap();

	return 0;
}
