#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <bgfx/bgfx.h>
#include <bx/readerwriter.h>
#include <bx/file.h>
#include <bgfx/platform.h>
#include <bgfx/defines.h>
#include "memory.h"


class Texture
{
public:
	Texture();
	Texture(int _stage);
	~Texture();
	//todo - replace with SOIL or something
	void loadTexture(const char* _name, uint32_t _flags, uint8_t _skip = 0, bgfx::TextureInfo* _info = NULL);
	void setStage(int _stage);
	void setTexture() const;


protected:
	bgfx::TextureHandle m_texture;
	bgfx::UniformHandle m_uniform;
	int m_stage;
};




#endif
