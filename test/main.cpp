#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define VQS_IMPLEMENTATION
#include "vk_queue_selector.h"

#include <cstdio>
#include <vector>

static void fakeGetPhysicalDeviceQueueFamilyProperties0(VkPhysicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties) {
	if(pQueueFamilyPropertyCount)
		*pQueueFamilyPropertyCount = 4;
	if(pQueueFamilyProperties) {
		pQueueFamilyProperties[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		pQueueFamilyProperties[0].queueCount = 10;

		pQueueFamilyProperties[1].queueFlags = VK_QUEUE_GRAPHICS_BIT;
		pQueueFamilyProperties[1].queueCount = 3;

		pQueueFamilyProperties[2].queueFlags = VK_QUEUE_TRANSFER_BIT;
		pQueueFamilyProperties[2].queueCount = 2;

		pQueueFamilyProperties[3].queueFlags = VK_QUEUE_COMPUTE_BIT;
		pQueueFamilyProperties[3].queueCount = 1;
	}
}
static VkResult fakeGetPhysicalDeviceSurfaceSupportKHR0(VkPhysicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR, VkBool32 *pSupported) {
	constexpr VkBool32 kSurfaceSupport[] = {0, 1, 0, 0};
	*pSupported = kSurfaceSupport[queueFamilyIndex];
	return VK_SUCCESS;
}

TEST_CASE("Large requirements and different priorities") {
	VqsVulkanFunctions functions = {};
	functions.vkGetPhysicalDeviceQueueFamilyProperties = fakeGetPhysicalDeviceQueueFamilyProperties0;
	functions.vkGetPhysicalDeviceSurfaceSupportKHR = fakeGetPhysicalDeviceSurfaceSupportKHR0;

	VqsQueryCreateInfo createInfo = {};
	createInfo.physicalDevice = NULL;
	VqsQueueRequirements requirements[] = {
		{VK_QUEUE_GRAPHICS_BIT, 1.0f, (VkSurfaceKHR)(main)},
		{VK_QUEUE_GRAPHICS_BIT, 0.8f, (VkSurfaceKHR)(main)},
		{VK_QUEUE_COMPUTE_BIT, 1.0f, (VkSurfaceKHR)(main)},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 1.0f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_COMPUTE_BIT, 0.5f, nullptr},
		{VK_QUEUE_TRANSFER_BIT, 1.0f, nullptr},
		{VK_QUEUE_TRANSFER_BIT, 1.0f, nullptr},
		{VK_QUEUE_TRANSFER_BIT, 1.0f, nullptr},
		{VK_QUEUE_TRANSFER_BIT, 1.0f, nullptr},
	};
	createInfo.queueRequirementCount = std::size(requirements);
	createInfo.pQueueRequirements = requirements;
	createInfo.pVulkanFunctions = &functions;

	VqsQuery query;
	REQUIRE( vqsCreateQuery(&createInfo, &query) == VK_SUCCESS );
	REQUIRE( vqsPerformQuery(query) == VK_SUCCESS );

	std::vector<VqsQueueSelection> selections;
	selections.resize(std::size(requirements));
	vqsGetQueueSelections(query, selections.data());

	printf("Selections:\n");
	for(const VqsQueueSelection &i : selections) {
		printf("{familyIndex:%d, queueIndex:%d, presentFamilyIndex:%d, presentQueueIndex:%d}\n", 
			   i.queueFamilyIndex, i.queueIndex, i.presentQueueFamilyIndex, i.presentQueueIndex);
	}

	uint32_t queueCreateInfoCount, queuePriorityCount;
	vqsEnumerateDeviceQueueCreateInfos(query, &queueCreateInfoCount, nullptr, &queuePriorityCount, nullptr);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueCreateInfoCount);
	std::vector<float> queuePriorities(queuePriorityCount);
	vqsEnumerateDeviceQueueCreateInfos(query, &queueCreateInfoCount, queueCreateInfos.data(), &queuePriorityCount, queuePriorities.data());

	printf("Creations:\n");
	for(const VkDeviceQueueCreateInfo &i : queueCreateInfos) {
		printf("{familyIndex:%u, queueCount:%u, queuePriorities:[", i.queueFamilyIndex, i.queueCount);
		for(uint32_t j = 0; j < i.queueCount; ++j) {
			printf("%.2f", i.pQueuePriorities[j]);
			if(j < i.queueCount - 1) printf(", ");
		}
		printf("]}\n");
	}

	vqsDestroyQuery(query);

	REQUIRE( queueCreateInfos[2].queueCount == 2 );
	REQUIRE( selections[11].queueFamilyIndex == 3 );
}
