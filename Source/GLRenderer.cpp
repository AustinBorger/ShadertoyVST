/*
  ==============================================================================

    GLRenderer.cpp
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin

  ==============================================================================
*/

#include <JuceHeader.h>
#include "GLRenderer.h"
#include <algorithm>

static const juce::String vert =
"#version 330\n"
"out vec2 texCoord;\n"
"\n"
"void main()\n"
"{\n"
"    float x = -1.0 + float((gl_VertexID & 1) << 2);\n"
"    float y = -1.0 + float((gl_VertexID & 2) << 1);\n"
"    texCoord.x = (x+1.0)*0.5;\n"
"    texCoord.y = (y+1.0)*0.5;\n"
"    gl_Position = vec4(x, y, 0, 1);\n"
"}\n";

static const juce::String copyFrag =
"#version 330\n"
"in vec2 texCoord;\n"
"out vec4 FragColor;\n"
"\n"
"uniform sampler2D visuTexture;\n"
"uniform float widthRatio;\n"
"uniform float heightRatio;\n"
"\n"
"void main() {\n"
    "FragColor = texture(visuTexture, vec2(texCoord.x / widthRatio, texCoord.y / heightRatio));\n"
"}\n";

//==============================================================================
GLRenderer::GLRenderer(ShadertoyAudioProcessor& processor,
                       juce::OpenGLContext &glContext)
 : processor(processor),
   glContext(glContext),
   programs(),
   copyProgram(glContext),
   uniformFloats(),
   uniformInts()
{
    setOpaque(true);
	glContext.setRenderer(this);
	glContext.attachTo(*this);
	glContext.setContinuousRepainting(true);
}

GLRenderer::~GLRenderer()
{
    glContext.detach();
}

void GLRenderer::newOpenGLContextCreated()
{
    if (!loadExtensions()) {
	    goto failure;
    }
    
    if (!buildCopyProgram()) {
        goto failure;
    }
    
    for (int i = 0; i < processor.getNumShaderFiles(); i++) {
        std::unique_ptr<juce::OpenGLShaderProgram> program(new juce::OpenGLShaderProgram(glContext));
        programs.emplace_back(std::move(program));
        resolutionIntrinsics.emplace_back();
        uniformFloats.emplace_back();
        uniformInts.emplace_back();
        if (!buildShaderProgram(i)) {
            goto failure;
        }
    }
    
    if (!createFramebuffer()) {
        goto failure;
    }
    
    validState = true;
    return;
    
failure:
    validState = false;
}

void GLRenderer::openGLContextClosing()
{
    resolutionIntrinsics.clear();
    uniformFloats.clear();
    uniformInts.clear();
    copyProgram.release();

    for (std::unique_ptr<juce::OpenGLShaderProgram> &program : programs) {
        program->release();
    }
    programs.clear();
}

void GLRenderer::renderOpenGL()
{
    jassert(juce::OpenGLHelpers::isContextActive());
    juce::OpenGLHelpers::clear(juce::Colours::black);

    if (validState) {
        double scaleFactor = glContext.getRenderingScale(); // DPI scaling
        int programIdx = processor.getProgramIdx();
        int backBufferWidth = (int)(getWidth() * scaleFactor);
        int backBufferHeight = (int)(getHeight() * scaleFactor);
        
        if (programIdx < programs.size()) {
            programs[programIdx]->use();
        
            for (int i = 0; i < uniformFloats[programIdx].size(); i++) {
                float val = processor.getUniformFloat(i);
                uniformFloats[programIdx][i]->set(val);
            }
        
            for (int i = 0; i < uniformInts[programIdx].size(); i++) {
                int val = processor.getUniformInt(i);
                uniformInts[programIdx][i]->set(val);
            }
            
            if (processor.getShaderFixedSizeBuffer(programIdx)) {
                int framebufferWidth = processor.getShaderFixedSizeWidth(programIdx);
                int framebufferHeight = processor.getShaderFixedSizeHeight(programIdx);

                if (resolutionIntrinsics[programIdx] != nullptr) {
                    resolutionIntrinsics[programIdx]->set((GLfloat)framebufferWidth,
                                                          (GLfloat)framebufferHeight);
                }

                /*
                 * First draw to fixed-size framebuffer
                 */
                glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
                glBindTexture(GL_TEXTURE_2D, mRenderTexture);
                glViewport(0, 0, framebufferWidth, framebufferHeight);
                glDrawArrays(GL_TRIANGLES, 0, 3);
        
                /*
                 * Now stretch to the render area
                 */
                copyProgram.use();

                widthRatio->set((float)mFramebufferWidth / (float)framebufferWidth);
                heightRatio->set((float)mFramebufferHeight / (float)framebufferHeight);
                
                glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, backBufferWidth, backBufferHeight);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            } else {
                if (resolutionIntrinsics[programIdx] != nullptr) {
                    resolutionIntrinsics[programIdx]->set((GLfloat)backBufferWidth,
                                                          (GLfloat)backBufferHeight);
                }

                /*
                 * Draw directly to back buffer
                 */
                glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, backBufferWidth, backBufferHeight);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
        } else {
            /*
             * Undefined program, just clear the back buffer
             */
            glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, backBufferWidth, backBufferHeight);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
}

void GLRenderer::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    if (validState) {
        // Empty
    }
}

bool GLRenderer::loadExtensions()
{
    const juce::String errorTitle = "Insufficient OpenGL version";

    glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)
	    juce::OpenGLHelpers::getExtensionFunction("glGetActiveUniform");
	if (glGetActiveUniform == nullptr) {
	    alertError(errorTitle, "Could not find glGetActiveUniform");
	    return false;
	}
	
	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)
	    juce::OpenGLHelpers::getExtensionFunction("glDrawBuffers");
	if (glDrawBuffers == nullptr) {
	    alertError(errorTitle, "Could not find glDrawBuffers");
	    return false;
	}
	
	return true;
}

bool GLRenderer::checkIntrinsicUniform(const juce::String &name,
                                       GLenum type,
                                       GLint size,
                                       bool &isIntrinsic,
                                       int programIdx)
{
    isIntrinsic = false;
    
    static const char *RESOLUTION_INTRINSIC_NAME = "iResolution";

    if (name == RESOLUTION_INTRINSIC_NAME) {
        if (type != GL_FLOAT_VEC2 || size != 1) {
            goto failure;
        }
        
        resolutionIntrinsics[programIdx] = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
            (new juce::OpenGLShaderProgram::Uniform(*programs[programIdx], RESOLUTION_INTRINSIC_NAME)));
        
        isIntrinsic = true;
        return true;
    }
    
    return true;
    
failure:
    alertError("Error reading uniforms",
               "Illegal use of intrinsic uniform name \"" + name + "\"");
    return false;
}

bool GLRenderer::buildShaderProgram(int idx)
{
    if (!programs[idx]->addVertexShader(vert) ||
	    !programs[idx]->addFragmentShader(processor.getShaderString(idx)) ||
	    !programs[idx]->link()) {
	    alertError("Error building program", programs[idx]->getLastError());
	    return false;
	}
	
	GLint count;
	glContext.extensions.glGetProgramiv(programs[idx]->getProgramID(), GL_ACTIVE_UNIFORMS, &count);
	
	GLchar name[256];
	GLsizei length;
	GLint size;
	GLenum type;
	for (GLint i = 0; i < count; i++) {
	    glGetActiveUniform(programs[idx]->getProgramID(), (GLuint)i, ARRAYSIZE(name),
	                       &length, &size, &type, name);
	    name[length] = '\0';

	    const juce::String nameStr = name;
	    bool isIntrinsic;
	    if (!checkIntrinsicUniform(nameStr, type, size, isIntrinsic, idx)) {
	        return false;
	    }
	    
	    if (!isIntrinsic) {
	        if (size != 1) {
	            juce::String message = "Parameter uniform \"";
	            message += name;
	            message += "\" cannot be an array.";
	            alertError("Error reading uniforms", message);
	            return false;
	        }
	        
	        if (type == GL_FLOAT) {
	            uniformFloats[idx].emplace_back(std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	                (new juce::OpenGLShaderProgram::Uniform(*programs[idx], name))));
	        } else if (type == GL_INT) {
	            uniformInts[idx].emplace_back(std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	                (new juce::OpenGLShaderProgram::Uniform(*programs[idx], name))));
	        } else {
	            juce::String message = "Parameter uniform \"";
	            message += name;
	            message += "\" of unrecognized type.";
	            alertError("Error reading uniforms", message);
	            return false;
	        }
	    }
	}
	
	return true;
}

bool GLRenderer::buildCopyProgram()
{
    bool shouldBuildCopyProgram = false;
    
    for (int i = 0; i < processor.getNumShaderFiles(); i++) {
        shouldBuildCopyProgram = shouldBuildCopyProgram || processor.getShaderFixedSizeBuffer(i);
    }
    
    if (!shouldBuildCopyProgram) {
        return true;
    }

    if (!copyProgram.addVertexShader(vert) ||
        !copyProgram.addFragmentShader(copyFrag) ||
        !copyProgram.link()) {
        alertError("Error building copy program", copyProgram.getLastError());
        return false;
    }
    
    GLint count;
	glContext.extensions.glGetProgramiv(copyProgram.getProgramID(), GL_ACTIVE_UNIFORMS, &count);
	if (count != 3) {
	    alertError("Unexpected number of uniforms in copyProgram",
	               "Unexpected number of uniforms (" + std::to_string(count) + ")");
	    return false;
	}
	
	GLchar name[256];
	GLsizei length;
	GLint size;
	GLenum type;
	for (GLint i = 0; i < count; i++) {
	    glGetActiveUniform(copyProgram.getProgramID(), (GLuint)i, ARRAYSIZE(name),
	                       &length, &size, &type, name);
	    name[length] = '\0';

	    const juce::String nameStr = name;
	    if (nameStr == "widthRatio") {
	        widthRatio = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	            (new juce::OpenGLShaderProgram::Uniform(copyProgram, name)));
	    } else if (nameStr == "heightRatio") {
	        heightRatio = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	            (new juce::OpenGLShaderProgram::Uniform(copyProgram, name)));
	    } else if (nameStr == "visuTexture") {
	        // Nothing
	    } else {
	        alertError("Unrecognized uniform in copyProgram", "Unrecognized uniform " + nameStr);
	        return false;
	    }
	}

    return true;
}

bool GLRenderer::createFramebuffer()
{
    // Figure out if we have to create one and how big it needs to be
    bool shouldCreateBuffer = false;
    mFramebufferWidth = 640;
    mFramebufferHeight = 360;
    
    for (int i = 0; i < processor.getNumShaderFiles(); i++) {
        shouldCreateBuffer = shouldCreateBuffer || processor.getShaderFixedSizeBuffer(i);
        if (processor.getShaderFixedSizeBuffer(i)) {
            mFramebufferWidth = max(mFramebufferWidth, processor.getShaderFixedSizeWidth(i));
            mFramebufferHeight = max(mFramebufferHeight, processor.getShaderFixedSizeHeight(i));
        }
    }
    
    if (!shouldCreateBuffer) {
        return true;
    }
    
    glContext.extensions.glGenFramebuffers(1, &mFramebuffer);
    glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glGenTextures(1, &mRenderTexture);
    glBindTexture(GL_TEXTURE_2D, mRenderTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mFramebufferWidth, mFramebufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glContext.extensions.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mRenderTexture, 0);
    
    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);
    
    if (glContext.extensions.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        alertError("Unable to construct framebuffer", "Failed to construct the framebuffer");
        return false;
    }

    return true;
}

void GLRenderer::alertError(const juce::String &title,
                            const juce::String &message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
	                                       title, message);
}
