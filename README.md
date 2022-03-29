# Vulkan Queue Selector
An single-header c library to select optimal vulkan queues.
## Usage
Check [vk_queue_selector.h](https://github.com/AdamYuan/VulkanQueueSelector/blob/main/vk_queue_selector.h) and [test/main.cpp](https://github.com/AdamYuan/VulkanQueueSelector/blob/main/test/main.cpp) for details.
```c++
VqsQueueRequirements requirements[] = {
    {VK_QUEUE_GRAPHICS_BIT, 1.0f, surface}, // "surface" is a VkSurfaceKHR, indicating a present queue is needed
    {VK_QUEUE_TRANSFER_BIT, 0.8f, nullptr},
    {VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT, 1.0f, nullptr}
};

VqsQueryCreateInfo createInfo = {};
createInfo.physicalDevice = physicalDevice;
createInfo.queueRequirementCount = std::size(requirements);
createInfo.pQueueRequirements = requirements;

// Specify vulkan functions if necessary
VqsVulkanFunctions functions = {};
functions.vkGetPhysicalDeviceQueueFamilyProperties = vkGetPhysicalDeviceQueueFamilyProperties;
functions.vkGetPhysicalDeviceSurfaceSupportKHR = vkGetPhysicalDeviceSurfaceSupportKHR;
ccreateInfo.pVulkanFunctions = &functions;

VqsQuery query;
vqsCreateQuery(&createInfo, &query); // Create a VqsQuery object, return VK_SUCCESS if success

vqsPerformQuery(query); // Perform the query, return VK_SUCCESS if the requirements can be met

// Get queue selections, the returned structures are defined as the following:
// typedef struct VqsQueueSelection {
//     uint32_t queueFamilyIndex, queueIndex;
//     uint32_t presentQueueFamilyIndex, presentQueueIndex;
// } VqsQueueSelection;
std::vector<VqsQueueSelection> selections;
selections.resize(std::size(requirements));
vqsGetQueueSelections(query, selections.data());

// Get an array of VkDeviceQueueCreateInfo
uint32_t queueCreateInfoCount, queuePriorityCount;
vqsEnumerateDeviceQueueCreateInfos(query, &queueCreateInfoCount, nullptr, &queuePriorityCount, nullptr);
std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueCreateInfoCount);
std::vector<float> queuePriorities(queuePriorityCount);
vqsEnumerateDeviceQueueCreateInfos(query, &queueCreateInfoCount, queueCreateInfos.data(), &queuePriorityCount, queuePriorities.data());

// Destroy the object
vqsDestroyQuery(query);
```
## Strategy
* The library will try to distribute **queue requirements** evenly into each queue family, which means the concurrent computation performance of modern GPUs can be utilized.  
* For all the queue families that meet a requirement, the ones with similar properties are preferred. Queue requirements with higher **priority** value is more likely to be assigned to an exclusive queue family.  
* Requirements for present queues are bound to a regular queue requirement since these two queues are preferred to be the same **VkQueue**. They will be separate only if a queue family that both support present and meet the regular requirement doesn't exist.
## Algorithm
* In this library, the queue selection problem is abstracted as a **binary graph minimum-cost flow problem**.
```
                                   Requirement 1 +-----+
     +----> Queue Family 1                             |
+------+                    INNER  Requirement 2 +--->-v--+
|SOURCE+--> Queue Family 2  EDGES                    |SINK|
+------+                    ...    Requirement 3 +--->-^--+
     +----> Queue Family 3                             |
                                   Requirement 4 +-----+
```
* In the binary graph, an inner edge indicates that a queue family can meet a requirement, whose **cost** is calculated based on requirement priority and the property differences.  
* The capacity of edges from source to queue families is synchronously added by one each time to ensure the requirements evenly being assigned.
