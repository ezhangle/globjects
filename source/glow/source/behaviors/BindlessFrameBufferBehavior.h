#pragma once

#include <GL/glew.h>

#include "AbstractFrameBufferBehavior.h"

namespace glow
{

class BindlessFrameBufferBehavior : public AbstractFrameBufferBehavior
{
public:
    virtual GLenum checkStatus(const FrameBufferObject * fbo) const override;
    virtual void setParameter(const FrameBufferObject * fbo, GLenum pname, GLint param) const override;
    virtual GLint getAttachmentParameter(const FrameBufferObject * fbo, GLenum attachment, GLenum pname) const override;
    virtual void attachTexture(const FrameBufferObject * fbo, GLenum attachment, Texture * texture, GLint level) const override;
    virtual void attachTexture1D(const FrameBufferObject * fbo, GLenum attachment, Texture * texture, GLint level) const override;
    virtual void attachTexture2D(const FrameBufferObject * fbo, GLenum attachment, Texture * texture, GLint level) const override;
    virtual void attachTexture3D(const FrameBufferObject * fbo, GLenum attachment, Texture * texture, GLint level, GLint layer) const override;
    virtual void attachTextureLayer(const FrameBufferObject * fbo, GLenum attachment, Texture * texture, GLint level, GLint layer) const override;
    virtual void attachRenderBuffer(const FrameBufferObject * fbo, GLenum attachment, RenderBufferObject * renderBuffer) const override;
};

} // namespace glow
