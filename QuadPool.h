#pragma once
namespace k10
{
	struct Vertex
	{
		static const VkVertexInputBindingDescription bindingDescription;
		static const vector<VkVertexInputAttributeDescription> attributeDescriptions;
		glm::vec2 position;
		glm::vec4 color;
	};
	class GfxBuffer
	{
	public:
		bool createBuffer(VkDevice d, VkPhysicalDevice pd,
						  VkDeviceSize size, 
						  VkBufferUsageFlags usageFlags,
						  VkMemoryPropertyFlags memPropFlags);
		void destroyBuffer();
		void mapMemory(void** data, VkDeviceSize offset, VkDeviceSize size);
		void unmapMemory();
		VkBuffer getBuffer() const;
	private:
		// Returns a uint32_t that represents the index of the physicalDevice's
		//	memory types that satisfies the params.
		// Returns 'numeric_limits<uint64_t>::max()' on failure.
		static uint64_t findMemoryType(uint32_t typeFilter, 
									   VkMemoryPropertyFlags properties,
									   VkPhysicalDevice pd);
	private:
		VkDevice device;
		VkBuffer buffer;
		VkDeviceMemory deviceMemory;
		VkDeviceSize bufferSize;
	};
	class QuadPool
	{
	public:
		using QuadId = uint32_t;
	public:
		bool fillPool(VkDevice d, VkPhysicalDevice pd, VkCommandPool cp,
					  size_t maxQuadCount);
		void drainPool();
		// returns the max value of QuadId if we have already reached the 
		//	maximum possible # of quads in the pool
		QuadId addQuad(vector<Vertex> const& vertices);
		void removeQuad(QuadId qid);
		void flushVertexStaging(VkQueue qMemoryTransfer);
		bool flushRequired() const;
		void issueCommands(VkCommandBuffer cb);
	private:
		static const Uint8 VERTICES_PER_QUAD;
		static const VkDeviceSize QUAD_VERTEX_DATA_SIZE;
		struct Quad
		{
			VkDeviceSize dataBufferOffset;
		};
		enum StagingQuadDataBits
		{
			STAGING_QUAD_DATA_BIT_POSITION = 0x01,
			STAGING_QUAD_DATA_BIT_COLOR    = 0x02,
			// prefer to select the ALL mask, since it should allow us to group
			//	together multiple staging data command transfers into a batch
			STAGING_QUAD_DATA_BIT_ALL      = 0x03
		};
		struct StagingQuad
		{
			VkDeviceSize dataBufferOffset;
			Uint8 stagingQuadDataBits = 0;
			// there is no need to store data in the staging quad array because
			//	currently I am just immediately storing the necessary data in
			//	a mapped staging buffer immediately after the changes are sent
			//	to the pool //
///			Vertex data[4];
		};
	private:
		VkDevice device;
		VkCommandPool commandPool;
		GfxBuffer quadDataBuffer;
		GfxBuffer stagingBufferVertices;
		size_t maxQuadCount;
		// maxQuadId is used to determine how many quads in the buffer should 
		//	be sent to the render pass command buffer.  Removed quads will 
		//	still attempt to be drawn, but we can assume they are degenerate
		//	geometry.  This is to prevent the need for having to initialize
		//	the entire quad data buffer to degenerate geometry when the pool
		//	gets filled, which would probably take a long time if our pool
		//	size is large!
		QuadId largestQuadCount;
		QuadId nextQuadId;
///		// this layer of indirection from QuadId=>bufferOffset is required if we want
///		//	the ability to defragment the quads so they are all at the beginning of
///		//	the buffer
///		std::unordered_map<QuadId, Quad> quads;
		// It shouldn't be necessary to defragment the quads so that they are 
		//	all aligned in GPU memory, as my current draw strategy is to just 
		//	allow potentially the entire buffer to be sent to a draw command 
		//	buffer. This strategy should work if I set quads that were 
		//	"removed" from the pool to just be degenerate geometry.
		std::unordered_set<QuadId> quadSet;
		// stagingQuads represents a collection of meta data describing the
		//	data that has already been added to the staging buffer which is
		//	waiting to be added to the quad data buffer.
		vector<StagingQuad> stagingQuads;
		VkFence stagingMemoryTransferFence;
	};
}
