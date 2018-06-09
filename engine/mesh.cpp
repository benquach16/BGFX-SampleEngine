#include "mesh.h"

#include <iostream>


Mesh::~Mesh()
{
	//clear textures
	for(unsigned i = 0; i < m_textures.size(); i++)
	{
		delete m_textures[i];
	}
}

namespace bgfx
{
	int32_t read(bx::ReaderI* _reader, bgfx::VertexDecl& _decl, bx::Error* _err = NULL);
}

void Mesh::load(bx::ReaderSeekerI* _reader)
{
#define BGFX_CHUNK_MAGIC_VB  BX_MAKEFOURCC('V', 'B', ' ', 0x1)
#define BGFX_CHUNK_MAGIC_IB  BX_MAKEFOURCC('I', 'B', ' ', 0x0)
#define BGFX_CHUNK_MAGIC_IBC BX_MAKEFOURCC('I', 'B', 'C', 0x0)
#define BGFX_CHUNK_MAGIC_PRI BX_MAKEFOURCC('P', 'R', 'I', 0x0)

	using namespace bx;
	using namespace bgfx;
	Group group;


	//bx::AllocatorI* allocator = new bx::DefaultAllocator;
	bx::DefaultAllocator allocator;
	uint32_t chunk;
	while (4 == bx::read(_reader, chunk) )
	{
		switch (chunk)
		{
		case BGFX_CHUNK_MAGIC_VB:
		{
			read(_reader, group.m_sphere);
			read(_reader, group.m_aabb);
			read(_reader, group.m_obb);

			read(_reader, m_decl);

			uint16_t stride = m_decl.getStride();

			uint16_t numVertices;
			read(_reader, numVertices);
			const bgfx::Memory* mem = bgfx::alloc(numVertices*stride);
			read(_reader, mem->data, mem->size);

			group.m_vbh = bgfx::createVertexBuffer(mem, m_decl);
		}
		break;

		case BGFX_CHUNK_MAGIC_IB:
		{
			uint32_t numIndices;
			read(_reader, numIndices);
			const bgfx::Memory* mem = bgfx::alloc(numIndices*2);
			read(_reader, mem->data, mem->size);
			group.m_ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case BGFX_CHUNK_MAGIC_IBC:
		{
			uint32_t numIndices;
			bx::read(_reader, numIndices);

			const bgfx::Memory* mem = bgfx::alloc(numIndices*2);

			uint32_t compressedSize;
			bx::read(_reader, compressedSize);

			void* compressedIndices = BX_ALLOC(&allocator, compressedSize);

			bx::read(_reader, compressedIndices, compressedSize);

			ReadBitstream rbs( (const uint8_t*)compressedIndices, compressedSize);
			DecompressIndexBuffer( (uint16_t*)mem->data, numIndices / 3, rbs);

			BX_FREE(&allocator, compressedIndices);

			group.m_ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case BGFX_CHUNK_MAGIC_PRI:
		{
			uint16_t len;
			read(_reader, len);

			std::string material;
			material.resize(len);
			read(_reader, const_cast<char*>(material.c_str() ), len);

			uint16_t num;
			read(_reader, num);

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				read(_reader, len);

				std::string name;
				name.resize(len);
				read(_reader, const_cast<char*>(name.c_str() ), len);

				Primitive prim;
				read(_reader, prim.m_startIndex);
				read(_reader, prim.m_numIndices);
				read(_reader, prim.m_startVertex);
				read(_reader, prim.m_numVertices);
				read(_reader, prim.m_sphere);
				read(_reader, prim.m_aabb);
				read(_reader, prim.m_obb);

				group.m_prims.push_back(prim);
			}

			m_groups.push_back(group);
			group.reset();
		}
		break;

		default:
			std::cout << "what the fuck" << std::endl;
			break;
		}
	}


}



void Mesh::submit(uint8_t _id, bgfx::ProgramHandle _program, const float* _mtx, uint64_t _state)
{
	if (BGFX_STATE_MASK == _state)
	{
		_state = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_MSAA
			;
	}

	for(unsigned i = 0 ; i < m_textures.size() ; i ++)
	{
		m_textures[i]->setTexture();
	}
	uint32_t cached = bgfx::setTransform(_mtx);
	for (GroupArray::const_iterator it = m_groups.begin(), itEnd = m_groups.end(); it != itEnd; ++it)
	{
		const Group& group = *it;
		bgfx::setTransform(cached);
		bgfx::setIndexBuffer(group.m_ibh);
		bgfx::setVertexBuffer(0, group.m_vbh);
		bgfx::setState(_state);
		bgfx::submit(_id, _program);
	}
}

void Mesh::addTexture(const char* _name, uint32_t _flags)
{
	//let index be stage
	int stage = m_textures.size();
	Texture *newTexture = new Texture(stage);
	newTexture->loadTexture(_name, _flags);
	m_textures.push_back(newTexture);
}

void Mesh::setTexture(int _index, const char* _name, uint32_t _flags)
{
	int stage = _index;
	Texture *newTexture = new Texture(stage);
	newTexture->loadTexture(_name, _flags);
	m_textures.push_back(newTexture);
}



Mesh* meshLoad(bx::ReaderSeekerI* _reader)
{
   	Mesh* mesh = new Mesh;
	mesh->load(_reader);
	return mesh;
}

Mesh* meshLoad(const char* _filePath)
{

	bx::FileReader reader;
	bx::open(&reader, _filePath);
	Mesh* mesh = meshLoad(&reader);
	bx::close(&reader);
	return mesh;
}


