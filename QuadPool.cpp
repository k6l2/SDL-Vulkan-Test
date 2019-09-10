#include "QuadPool.h"
const Uint8 k10::QuadPool::VERTICES_PER_QUAD = 6;
const VkDeviceSize k10::QuadPool::QUAD_VERTEX_DATA_SIZE = sizeof(Vertex) * VERTICES_PER_QUAD;
const VkVertexInputBindingDescription k10::Vertex::bindingDescription = {
	0,// binding
	sizeof(Vertex),
	VK_VERTEX_INPUT_RATE_VERTEX
};
const vector<VkVertexInputAttributeDescription> k10::Vertex::attributeDescriptions = {
	{0,//location
		0,//binding
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(Vertex, position)},
	{1,//location
		0,//binding
		VK_FORMAT_R32G32B32A32_SFLOAT,
		offsetof(Vertex, color)}
};
bool k10::GfxBuffer::createBuffer(VkDevice d, VkPhysicalDevice pd, 
								  VkDeviceSize size,
								  VkBufferUsageFlags usageFlags,
								  VkMemoryPropertyFlags memPropFlags)
{
	device = d;
	bufferSize = size;
	const VkBufferCreateInfo bufferCreateInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		size,
		usageFlags,
		VK_SHARING_MODE_EXCLUSIVE,
		0,// queue family index count
		nullptr // queue family indices (unused if sharing mode is exclusive)
	};
	if (vkCreateBuffer(device, &bufferCreateInfo,
					   nullptr, &buffer) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create Vulkan buffer!\n");
		SDL_assert(false);
		return false;
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(d, buffer, &memRequirements);
	const uint64_t memTypeIndex = findMemoryType(
		memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		pd);
	if (memTypeIndex == numeric_limits<uint64_t>::max())
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to find suitable memory type for vertex buffer!\n");
		SDL_assert(false);
		return false;
	}
	const VkMemoryAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,// pNext
		memRequirements.size,
		static_cast<uint32_t>(memTypeIndex)
	};
	if (vkAllocateMemory(d, 
						 &allocInfo, 
						 nullptr, 
						 &deviceMemory) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to allocate memory for vertex buffer!\n");
		SDL_assert(false);
		return false;
	}
	vkBindBufferMemory(d, buffer, deviceMemory, 0);
	return true;
}
void k10::GfxBuffer::destroyBuffer()
{
	vkDestroyBuffer(device, buffer, nullptr);
	vkFreeMemory(device, deviceMemory, nullptr);
}
void k10::GfxBuffer::mapMemory(void** data, VkDeviceSize offset, VkDeviceSize size)
{
	vkMapMemory(device, deviceMemory, offset, size, VkMemoryMapFlags(0), data);
}
void k10::GfxBuffer::unmapMemory()
{
	vkUnmapMemory(device, deviceMemory);
}
VkBuffer k10::GfxBuffer::getBuffer() const
{
	return buffer;
}
uint64_t k10::GfxBuffer::findMemoryType(uint32_t typeFilter,
									    VkMemoryPropertyFlags properties,
									    VkPhysicalDevice pd)
{
	VkPhysicalDeviceMemoryProperties physicalMemProps;
	vkGetPhysicalDeviceMemoryProperties(pd, &physicalMemProps);
	for (uint32_t t = 0; t < physicalMemProps.memoryTypeCount; t++)
	{
		if ((typeFilter & (1 << t)) &&
			(physicalMemProps.memoryTypes[t].propertyFlags & properties) == properties)
		{
			return t;
		}
	}
	return numeric_limits<uint64_t>::max();
}
bool k10::QuadPool::fillPool(VkDevice d, VkPhysicalDevice pd, VkCommandPool cp,
							 size_t mqc)
{
	device = d;
	commandPool = cp;
	maxQuadCount = mqc;
	nextQuadId = 0;
	largestQuadCount = 0;
	quadSet.clear();
	stagingQuads.clear();
	const VkDeviceSize dataBufferSize = 
		static_cast<VkDeviceSize>(QUAD_VERTEX_DATA_SIZE * maxQuadCount);
	if (!quadDataBuffer.createBuffer(d, pd, dataBufferSize,
									 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
										VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create quad pool data buffer!\n");
		return false;
	}
	const VkDeviceSize stagingBufferVerticesSize =
		static_cast<VkDeviceSize>(QUAD_VERTEX_DATA_SIZE * maxQuadCount);
	if(!stagingBufferVertices.createBuffer(d, pd, stagingBufferVerticesSize,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
										   	  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create quad pool vertex staging buffer!\n");
		return false;
	}
	const VkFenceCreateInfo fenceCreateInfo = {
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,// pNext
		VK_FENCE_CREATE_SIGNALED_BIT // flags
	};
	if (vkCreateFence(device,
					  &fenceCreateInfo,
					  nullptr,
					  &stagingMemoryTransferFence) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to allocate stagingMemoryTransferFence!\n");
		return false;
	}
	return true;
}
void k10::QuadPool::drainPool()
{
	vkDestroyFence(device, stagingMemoryTransferFence, nullptr);
	quadDataBuffer.destroyBuffer();
	stagingBufferVertices.destroyBuffer();
}
k10::QuadPool::QuadId k10::QuadPool::addQuad(vector<Vertex> const& vertices)
{
	SDL_assert(vertices.size() == VERTICES_PER_QUAD);
	if (quadSet.size() >= maxQuadCount)
	{
		SDL_Log("Aborting attempt to add a quad to a filled pool.\n");
		SDL_assert(false);
		return numeric_limits<QuadId>::max();
	}
	while (quadSet.find(nextQuadId) != quadSet.end())
	{
		nextQuadId = (nextQuadId + 1) % maxQuadCount;
	}
	// at this point, we can guarantee that the nextQuadId is valid //
	quadSet.insert(nextQuadId);
	if (quadSet.size() > static_cast<size_t>(largestQuadCount))
	{
		largestQuadCount = static_cast<uint32_t>(quadSet.size());
	}
	// add the vertex data into the mapped staging buffer //
	void* stagingData;
	///TODO: change the vertex data offset once we interleave vertex data w/ index & uniform data~
	const VkDeviceSize newQuadVertexDataOffset = nextQuadId*QUAD_VERTEX_DATA_SIZE;
	stagingBufferVertices.mapMemory(&stagingData, 
									newQuadVertexDataOffset, 
									QUAD_VERTEX_DATA_SIZE);
	memcpy(stagingData, vertices.data(), static_cast<size_t>(QUAD_VERTEX_DATA_SIZE));
	stagingBufferVertices.unmapMemory();
	stagingQuads.push_back({newQuadVertexDataOffset, STAGING_QUAD_DATA_BIT_ALL });
	// prep the nextQuadId for the most likely next id to reduce # of set 
	//	lookups //
	const QuadId retVal = nextQuadId;
	nextQuadId = (nextQuadId + 1) % maxQuadCount;
	return retVal;
}
void k10::QuadPool::removeQuad(QuadId qid)
{
	auto quadSetIt = quadSet.find(qid);
	if (quadSetIt == quadSet.end())
	{
		SDL_Log("WARNING: trying to remove quad that doesn't exist!\n");
		SDL_assert(false);
		return;
	}
	SDL_assert(false);
	///TODO
	quadSet.erase(quadSetIt);
}
void k10::QuadPool::flushVertexStaging(VkQueue qMemoryTransfer)
{
	if (stagingQuads.empty())
	{
		return;
	}
	VkCommandBuffer memoryCommandBuffer;
	const VkCommandBufferAllocateInfo commandBufferAllocInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,// pNext
		commandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1 // command buffer count
	};
	vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &memoryCommandBuffer);
	const VkCommandBufferBeginInfo commandBufferBeginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,// pNext
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr // inheritance info pointer
	};
	vkBeginCommandBuffer(memoryCommandBuffer, &commandBufferBeginInfo);
	///TODO: maybe do some more work here to minimize the # of copy buffer 
	///	commands we have to issue here (need to test for performance once
	///	everything is up and running...
	vector<VkBufferCopy> bufferCopyRegions(stagingQuads.size());
	for (size_t r = 0; r < bufferCopyRegions.size(); r++)
	{
		StagingQuad const& sq = stagingQuads[r];
		if (sq.stagingQuadDataBits != STAGING_QUAD_DATA_BIT_ALL)
		{
			SDL_Log("I haven't implemented partial Vertex updates yet.\n");
			SDL_assert(false);
			return;
		}
		const VkBufferCopy copyRegion = {
			sq.dataBufferOffset,// src offset
			sq.dataBufferOffset,// dst offset
			QUAD_VERTEX_DATA_SIZE
		};
		bufferCopyRegions[r] = copyRegion;
	}
	vkCmdCopyBuffer(memoryCommandBuffer, 
					stagingBufferVertices.getBuffer(),
					quadDataBuffer.getBuffer(), 
					static_cast<uint32_t>(bufferCopyRegions.size()), 
					bufferCopyRegions.data());
	stagingQuads.clear();
	vkEndCommandBuffer(memoryCommandBuffer);
	const VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,// pNext
		0,// wait semaphore count
		nullptr,// wait semaphores
		nullptr,// wait dst stage mask
		1,// command buffer count
		&memoryCommandBuffer,
		0,// signal semaphore count
		nullptr // signal semaphores
	};
	vkResetFences(device, 1, &stagingMemoryTransferFence);
	vkQueueSubmit(qMemoryTransfer, 1, &submitInfo, stagingMemoryTransferFence);
	///vkQueueWaitIdle(qMemoryTransfer);
	vkWaitForFences(device, 1, &stagingMemoryTransferFence, VK_TRUE, UINT64_MAX);
	vkFreeCommandBuffers(device, commandPool, 1, &memoryCommandBuffer);
}
bool k10::QuadPool::flushRequired() const
{
	return !stagingQuads.empty();
}
void k10::QuadPool::issueCommands(VkCommandBuffer cb)
{
	if (largestQuadCount <= 0)
	{
		return;
	}
	VkBuffer vertexBuffers[] = { quadDataBuffer.getBuffer() };
	VkDeviceSize vbOffsets[] = { 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, vbOffsets);
	vkCmdDraw(cb, static_cast<uint32_t>(largestQuadCount * VERTICES_PER_QUAD), 1, 0, 0);
}