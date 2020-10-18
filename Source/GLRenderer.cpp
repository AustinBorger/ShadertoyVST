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
   copyProgram(glContext)
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
        programData.emplace_back();
        programData.back().program = std::move(program);
        if (!buildShaderProgram(i)) {
            goto failure;
        }
    }
    
    if (!createFramebuffer()) {
        goto failure;
    }

    processor.addMidiListener(this);
    midiCollector.reset(44100.f);
    firstRender = 0.0;
    memset(keyDownLast, 0, sizeof(keyDownLast));
    memset(keyUpLast, 0, sizeof(keyUpLast));
    
    validState = true;
    return;
    
failure:
    validState = false;
}

void GLRenderer::openGLContextClosing()
{
    processor.removeMidiListener(this);

    copyProgram.release();

    for (auto &item : programData) {
        item.program->release();
    }
    programData.clear();
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

        double now = juce::Time::getMillisecondCounterHiRes();
        if (firstRender > 0.0) {
            juce::MidiBuffer midiBuffer;
            double elapsedMs = now - firstRender;
            int newSamplePos = int((elapsedMs / 1000.0) * 44100.0);
            int numSamples = newSamplePos - samplePos;

            midiCollector.removeNextBlockOfMessages(midiBuffer, numSamples);
            for (auto metadata : midiBuffer) {
                const juce::MidiMessage &message = metadata.getMessage();
                double timestamp = message.getTimeStamp();
                if (message.isNoteOn()) {
                    keyDownLast[message.getNoteNumber()] = (double(samplePos) + timestamp) / 44100.0;
                } else if (message.isNoteOff()) {
                    keyUpLast[message.getNoteNumber()] = (double(samplePos) + timestamp) / 44100.0;
                }
            }

            samplePos = newSamplePos;
        } else {
            firstRender = now;
            samplePos = 0;
        }
        
        if (programIdx < programData.size()) {
            ProgramData &program = programData[programIdx];
            program.program->use();
        
            for (int i = 0; i < program.uniformFloats.size(); i++) {
                float val = processor.getUniformFloat(i);
                program.uniformFloats[i]->set(val);
            }
        
            for (int i = 0; i < program.uniformInts.size(); i++) {
                int val = processor.getUniformInt(i);
                program.uniformInts[i]->set(val);
            }

            if (program.keyDownIntrinsic != nullptr) {
                GLfloat vals[MIDI_NUM_KEYS] = { };
                for (int i = 0; i < MIDI_NUM_KEYS; i++) {
                    vals[i] = (GLfloat)keyDownLast[i];
                }
                program.keyDownIntrinsic->set(vals, MIDI_NUM_KEYS);
            }

            if (program.keyUpIntrinsic != nullptr) {
                GLfloat vals[MIDI_NUM_KEYS] = { };
                for (int i = 0; i < MIDI_NUM_KEYS; i++) {
                    vals[i] = (GLfloat)keyUpLast[i];
                }
                program.keyUpIntrinsic->set(vals, MIDI_NUM_KEYS);
            }
            
            if (processor.getShaderFixedSizeBuffer(programIdx)) {
                int framebufferWidth = processor.getShaderFixedSizeWidth(programIdx);
                int framebufferHeight = processor.getShaderFixedSizeHeight(programIdx);

                if (program.resolutionIntrinsic != nullptr) {
                    program.resolutionIntrinsic->set((GLfloat)framebufferWidth,
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
                if (program.resolutionIntrinsic != nullptr) {
                    program.resolutionIntrinsic->set((GLfloat)backBufferWidth,
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

void GLRenderer::handleMidiMessage(const juce::MidiMessage &message)
{
    midiCollector.addMessageToQueue(message);
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
    ProgramData &program = programData[programIdx];
    isIntrinsic = false;
    
    static const char *RESOLUTION_INTRINSIC_NAME = "iResolution";
    static const char *KEY_DOWN_INTRINSIC_NAME = "iKeyDown[0]";
    static const char *KEY_UP_INTRINSIC_NAME = "iKeyUp[0]";

    if (name == RESOLUTION_INTRINSIC_NAME) {
        if (type != GL_FLOAT_VEC2 || size != 1) {
            goto failure;
        }
        
        program.resolutionIntrinsic = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
            (new juce::OpenGLShaderProgram::Uniform(*program.program, RESOLUTION_INTRINSIC_NAME)));
        
        isIntrinsic = true;
        return true;
    } else if (name == KEY_DOWN_INTRINSIC_NAME) {
        if (type != GL_FLOAT || size != MIDI_NUM_KEYS) {
            goto failure;
        }

        program.keyDownIntrinsic = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
            (new juce::OpenGLShaderProgram::Uniform(*program.program, KEY_DOWN_INTRINSIC_NAME)));

        isIntrinsic = true;
        return true;
    } else if (name == KEY_UP_INTRINSIC_NAME) {
        if (type != GL_FLOAT || size != MIDI_NUM_KEYS) {
            goto failure;
        }

        program.keyUpIntrinsic = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
            (new juce::OpenGLShaderProgram::Uniform(*program.program, KEY_UP_INTRINSIC_NAME)));

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
    ProgramData &program = programData[idx];

    if (!program.program->addVertexShader(vert) ||
	    !program.program->addFragmentShader(processor.getShaderString(idx)) ||
	    !program.program->link()) {
	    alertError("Error building program", program.program->getLastError());
	    return false;
	}
	
	GLint count;
	glContext.extensions.glGetProgramiv(program.program->getProgramID(), GL_ACTIVE_UNIFORMS, &count);
	
	GLchar name[256];
	GLsizei length;
	GLint size;
	GLenum type;
	for (GLint i = 0; i < count; i++) {
	    glGetActiveUniform(program.program->getProgramID(), (GLuint)i, ARRAYSIZE(name),
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
	            program.uniformFloats.emplace_back(std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	                (new juce::OpenGLShaderProgram::Uniform(*program.program, name))));
	        } else if (type == GL_INT) {
	            program.uniformInts.emplace_back(std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	                (new juce::OpenGLShaderProgram::Uniform(*program.program, name))));
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
