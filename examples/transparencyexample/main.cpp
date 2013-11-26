#include <GL\glew.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <glow\Program.h>
#include <glow\FrameBufferObject.h>
#include <glow\Texture.h>
#include <glow\RenderBufferObject.h>
#include <glow\DebugMessageOutput.h>

#include <glowwindow\ContextFormat.h>
#include <glowwindow\Window.h>
#include <glowwindow\MainLoop.h>
#include <glowwindow\Context.h>
#include <glowwindow\WindowEventHandler.h>

#include <glowutils\UnitCube.h>
#include <glowutils\File.h>
#include <glowutils\Camera.h>
#include <glowutils\AbstractCoordinateProvider.h>
#include <glowutils\WorldInHandNavigation.h>
#include <glowutils\FileRegistry.h>
#include <glowutils\ScreenAlignedQuad.h>


typedef struct CubeUniformAttributes
{
	glm::vec3 position;
	glm::vec4 color;

} CubeUniformAttributes;

typedef struct TransparencyAlgorithmData {
	glow::Program* program;
	glow::FrameBufferObject* fbo;
	glow::Texture* colorTex;
	glow::RenderBufferObject* depth;
} TransparencyAlgorithmData;

class EventHandler : public glowwindow::WindowEventHandler, glowutils::AbstractCoordinateProvider {

private:
	TransparencyAlgorithmData m_algos[2];

	glow::Program* m_screenAlignedQuadProgram;

	glowutils::Camera* m_camera;
	glowutils::UnitCube* m_cube;
	glowutils::WorldInHandNavigation m_nav;
	glowutils::AxisAlignedBoundingBox m_aabb;
	glowutils::ScreenAlignedQuad* m_quad;

	glow::Texture* createColorTex() {
		glow::Texture* color = new glow::Texture(GL_TEXTURE_2D);
		color->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		color->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		color->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		color->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		color->setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		return color;
	}

public:

	void initialize(glowwindow::Window & window) override {
		glow::DebugMessageOutput::enable();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		// Algo1: GL_BLEND
		m_algos[0].program = new glow::Program();
		m_algos[0].program->attach(glowutils::createShaderFromFile(GL_FRAGMENT_SHADER, "data/transparency/normal.frag"));
		m_algos[0].program->attach(glowutils::createShaderFromFile(GL_VERTEX_SHADER, "data/transparency/normal.vert"));

		m_algos[0].colorTex = createColorTex();
		m_algos[0].fbo = new glow::FrameBufferObject();
		m_algos[0].fbo->attachTexture2D(GL_COLOR_ATTACHMENT0, m_algos[0].colorTex);
		m_algos[0].depth = new glow::RenderBufferObject();
		m_algos[0].fbo->attachRenderBuffer(GL_DEPTH_ATTACHMENT, m_algos[0].depth);
		m_algos[0].fbo->setDrawBuffer(GL_COLOR_ATTACHMENT0);

		// Algo2: A-Buffer
		m_algos[1].program = new glow::Program();
		m_algos[1].program->attach(glowutils::createShaderFromFile(GL_FRAGMENT_SHADER, "data/transparency/transparency.frag"));
		m_algos[1].program->attach(glowutils::createShaderFromFile(GL_VERTEX_SHADER, "data/transparency/transparency.vert"));

		m_algos[1].colorTex = createColorTex();
		m_algos[1].fbo = new glow::FrameBufferObject();
		m_algos[1].fbo->attachTexture2D(GL_COLOR_ATTACHMENT0, m_algos[1].colorTex);
		m_algos[1].depth = new glow::RenderBufferObject();
		m_algos[1].fbo->attachRenderBuffer(GL_DEPTH_ATTACHMENT, m_algos[1].depth);
		m_algos[1].fbo->setDrawBuffer(GL_COLOR_ATTACHMENT0);

		// Setup the scene
		m_algos[0].program->getAttributeLocation("a_normal");
		m_algos[0].program->getAttributeLocation("a_vertex");

		// attributes a_vertex and a_normal must have the same location in both shaders so m_cube can be used for both
		assert(m_algos[0].program->getAttributeLocation("a_vertex") == m_algos[1].program->getAttributeLocation("a_vertex"));
		assert(m_algos[0].program->getAttributeLocation("a_normal") == m_algos[1].program->getAttributeLocation("a_normal"));

		m_cube = new glowutils::UnitCube(m_algos[0].program->getAttributeLocation("a_vertex"), m_algos[0].program->getAttributeLocation("a_normal"));

		m_camera = new glowutils::Camera(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Setup the screen aligned quad stuff
		m_screenAlignedQuadProgram = new glow::Program();
		m_screenAlignedQuadProgram->attach(glowutils::createShaderFromFile(GL_FRAGMENT_SHADER, "data/transparency/quad.frag"));
		m_screenAlignedQuadProgram->attach(glowutils::createShaderFromFile(GL_VERTEX_SHADER, "data/transparency/quad.vert"));
		m_quad = new glowutils::ScreenAlignedQuad(m_screenAlignedQuadProgram);

		m_aabb.extend(glm::vec3(-1.f, -0.5f, -10.5f));
		m_aabb.extend(glm::vec3(0.f, 0.5f, -5.5));

		m_nav.setCamera(m_camera);
		m_nav.setCoordinateProvider(this);
		m_nav.setBoundaryHint(m_aabb);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		CheckGLError();
	}

	void paintEvent(glowwindow::PaintEvent& event) override {

		int width = event.window()->width();
		int height = event.window()->height();

		const CubeUniformAttributes cubes[3] = {
			CubeUniformAttributes{ glm::vec3(0.0f, 0.0f, -10.0f), glm::vec4(1.0, 0.0, 0.0, 0.3) },
			CubeUniformAttributes{ glm::vec3(0.0f, -0.25f, -8.0f), glm::vec4(0.0, 1.0, 0.0, 0.3) },
			CubeUniformAttributes{ glm::vec3(0.0f, -0.5f, -6.0f), glm::vec4(0.0, 0.0, 1.0, 0.3) }
		};

		// STAGE1 - Draw the cubes for each transparency algorithm into the algorithms fbo
		glViewport(0, 0, width, height * 2);
		m_camera->setViewport(width, height * 2);
		TransparencyAlgorithmData current;
		for (int i = 0; i < 2; i++) {
			current = m_algos[i];
			current.fbo->bind();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			CheckGLError();
			
			current.program->setUniform("viewprojectionmatrix", m_camera->viewProjection());
			current.program->setUniform("normalmatrix", m_camera->normal());
			current.program->use();

			if (i == 0) glEnable(GL_BLEND);
			CheckGLError();

			for (int c = 0; c < 3; c++) {
				current.program->setUniform("modelmatrix", glm::translate<float>(cubes[c].position));
				current.program->setUniform("color", cubes[c].color);
				m_cube->draw();
			}

			glDisable(GL_BLEND);
			CheckGLError();

			current.fbo->unbind();
		}

		// STAGE2 - Draw the texture of each algorithm onto the screen aligned quad
		glViewport(0, 0, width, height);
		
		glDisable(GL_DEPTH_TEST);
		CheckGLError();

		glDepthMask(GL_FALSE);
		CheckGLError();

		m_screenAlignedQuadProgram->setUniform("one", 0);
		m_screenAlignedQuadProgram->setUniform("two", 1);
		m_algos[0].colorTex->bind(GL_TEXTURE0);
		m_algos[1].colorTex->bind(GL_TEXTURE1);

		m_screenAlignedQuadProgram->use();

		m_quad->draw();

		m_algos[0].colorTex->unbind(GL_TEXTURE1);
		m_algos[1].colorTex->unbind(GL_TEXTURE0);

		glEnable(GL_DEPTH_TEST);
		CheckGLError();

		glDepthMask(GL_TRUE);
		CheckGLError();

		
	}

	void resizeEvent(glowwindow::ResizeEvent & event) override {
		int width = event.width();
		int height = event.height();

		int numAlgos = 2;
		int result = glow::FrameBufferObject::defaultFBO()->getAttachmentParameter(GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE);

		for (TransparencyAlgorithmData d : m_algos) {
			d.colorTex->image2D(0, GL_RGBA32F, width, height * numAlgos, 0, GL_RGBA, GL_FLOAT, nullptr);
			d.depth->storage(result == 16 ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT, width, height * numAlgos);
		}
		
	}

	virtual void idle(glowwindow::Window & window) override
	{
		//window.repaint();
	}

	virtual const float depthAt(const glm::ivec2 & windowCoordinates) override
	{
		return glowutils::AbstractCoordinateProvider::depthAt(*m_camera, GL_DEPTH_COMPONENT, windowCoordinates);
	}

	virtual const glm::vec3 objAt(const glm::ivec2 & windowCoordinates) override
	{
		return unproject(*m_camera, static_cast<GLenum>(GL_DEPTH_COMPONENT), windowCoordinates);
	}

	virtual const glm::vec3 objAt(const glm::ivec2 & windowCoordinates, const float depth) override
	{
		return unproject(*m_camera, depth, windowCoordinates);
	}

	virtual const glm::vec3 objAt(const glm::ivec2 & windowCoordinates, const float depth, const glm::mat4 & viewProjectionInverted) override
	{
		return unproject(*m_camera, viewProjectionInverted, depth, windowCoordinates);
	}

	virtual void mousePressEvent(glowwindow::MouseEvent & event) override
	{
		switch (event.button())
		{
		case GLFW_MOUSE_BUTTON_LEFT:
			m_nav.panBegin(event.pos());
			event.accept();
			event.window()->repaint();
			break;

		case GLFW_MOUSE_BUTTON_RIGHT:
			m_nav.rotateBegin(event.pos());
			event.accept();
			event.window()->repaint();
			break;
		}
	}

	virtual void mouseMoveEvent(glowwindow::MouseEvent & event) override
	{
		switch (m_nav.mode())
		{
		case glowutils::WorldInHandNavigation::PanInteraction:
			m_nav.panProcess(event.pos());
			event.accept();
			event.window()->repaint();
			break;

		case glowutils::WorldInHandNavigation::RotateInteraction:
			m_nav.rotateProcess(event.pos());
			event.accept();
			event.window()->repaint();
		}
	}

	virtual void mouseReleaseEvent(glowwindow::MouseEvent & event) override
	{
		switch (event.button())
		{
		case GLFW_MOUSE_BUTTON_LEFT:
			m_nav.panEnd();
			event.accept();
			event.window()->repaint();
			break;

		case GLFW_MOUSE_BUTTON_RIGHT:
			m_nav.rotateEnd();
			event.accept();
			event.window()->repaint();
			break;
		}
	}

	virtual void keyPressEvent(glowwindow::KeyEvent & event) override
	{
		const float d = 0.08f;

		switch (event.key())
		{
		case GLFW_KEY_F5:
			glowutils::FileRegistry::instance().reloadAll();
			break;
		}
	}

};

int main(int argc, char* argv[]) {

	glowwindow::ContextFormat format;
	format.setVersion(4, 3);
	format.setDepthBufferSize(16);
	//format.setSamples(4);

	glowwindow::Window window;

	if (!window.create(format, "Transparency")) return 1;
	window.context()->setSwapInterval(glowwindow::Context::VerticalSyncronization);
	window.setEventHandler(new EventHandler());
	window.show();
	return glowwindow::MainLoop::run();
	
}