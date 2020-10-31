/*
  ==============================================================================

    GLRenderer.cpp
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#include <JuceHeader.h>
#include "GLRenderer.h"
#include "PluginEditor.h"
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
                       ShadertoyAudioProcessorEditor &editor,
                       juce::OpenGLContext &glContext)
 : processor(processor),
   editor(editor),
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

    firstRender = 0.0;
    prevRender = 0.0;
    firstAudioTimestamp = -1.0;

#if GLRENDER_LOG_FPS == 1
    avgFPS = 0.0;
    lastFPSLog = 0.0;
#endif

    for (int i = 0; i < MIDI_NUM_KEYS; i++) {
        keyDownLast[i] = -1.0f;
        keyUpLast[i] = -1.0f;
    }

    processor.addAudioListener(this);
    
    validState = true;
    return;
    
failure:
    validState = false;
}

void GLRenderer::openGLContextClosing()
{
    processor.removeAudioListener(this);

    copyProgram.release();

    for (auto &item : programData) {
        item.program->release();
    }
    programData.clear();

    midiFrames = { };

    audioChannel0 = nullptr;
    sizeAudioChannel0 = 0;
    audioChannel1 = nullptr;
    sizeAudioChannel1 = 0;
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
        double elapsedSeconds;
        double currentAudioTimestamp = 0.0;
        double sampleRate = 44100.0;

        if (firstRender < 0.001) {
            firstRender = now;
        }
        elapsedSeconds = (now - firstRender) / 1000.0;

#if GLRENDER_LOG_FPS == 1
        if (prevRender > 0.0) {
            double instFPS = 1000.0 / (now - prevRender);
            if (avgFPS > 0.0) {
                const double a = 0.96;
                avgFPS = avgFPS * a + instFPS * (1.0 - a);
            }  else {
                avgFPS = instFPS;
            }
        }

        if (avgFPS > 0.0) {
            if (now - lastFPSLog > 1000.0) {
                editor.logDebugMessage("FPS: " + std::to_string(avgFPS));
                lastFPSLog = now;
            }
        }
#endif

        mutex.enter();

        if (firstAudioTimestamp >= 0.0) {
            currentAudioTimestamp = firstAudioTimestamp + elapsedSeconds - DELAY_LATENCY;
            sampleRate = mSampleRate;
            while (!midiFrames.empty() && midiFrames.front().timestamp <= currentAudioTimestamp) {
                MidiFrame &midiFrame = midiFrames.front();
                for (auto metadata : midiFrame.buffer) {
                    const juce::MidiMessage &message = metadata.getMessage();
                    if (message.isNoteOn()) {
                        keyDownLast[message.getNoteNumber()] = midiFrame.timestamp;
                    } else if (message.isNoteOff()) {
                        keyUpLast[message.getNoteNumber()] = midiFrame.timestamp;
                    }
                }
                midiFrames.pop();
            }
        }
        
        if (programIdx < programData.size()) {
            ProgramData &program = programData[programIdx];
            program.program->use();

            if (program.audioChannel0 != nullptr && audioChannel0 != nullptr) {
                GLint sizeDiff = this->sizeAudioChannel0 - program.sizeAudioChannel0;
                program.audioChannel0->set(this->audioChannel0.get() + sizeDiff - (GLint)(sampleRate * DELAY_LATENCY),
                                           program.sizeAudioChannel0);
            }

            if (program.audioChannel1 != nullptr && audioChannel1 != nullptr) {
                GLint sizeDiff = this->sizeAudioChannel1 - program.sizeAudioChannel1;
                program.audioChannel1->set(this->audioChannel1.get() + sizeDiff - (GLint)(sampleRate * DELAY_LATENCY),
                                           program.sizeAudioChannel1);
            }

            mutex.exit();
        
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

            if (program.timeIntrinsic != nullptr) {
                program.timeIntrinsic->set((GLfloat)currentAudioTimestamp);
            }

            if (program.sampleRateIntrinsic != nullptr) {
                program.sampleRateIntrinsic->set((GLfloat)sampleRate);
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
            mutex.exit();

            /*
             * Undefined program, just clear the back buffer
             */
            glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, backBufferWidth, backBufferHeight);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        prevRender = now;
    }
}

/*
 * AdvanceAudioBuffer
 *    Updates the audio buffer with more current samples.
 */
static void AdvanceAudioBuffer(float *dst,       // The history buffer
                               int dstSize,      // The size of the history buffer
                               const float *src, // The buffer filled with new samples
                               int srcSize)      // The number of new samples
{
    int numSamplesReused = max(0, dstSize - srcSize);
    int numSamplesCopied = dstSize - numSamplesReused;
    int numSamplesSkipped = max(0, srcSize - dstSize);
    if (numSamplesReused > 0) {
        memmove(dst, dst + srcSize, numSamplesReused * sizeof(float));
    }
    memcpy(dst + numSamplesReused, src + numSamplesSkipped,
           numSamplesCopied * sizeof(float));
}

/*
 * GLRenderer::handleAudioFrame
 *    Handles incoming audio samples / midi events.
 */
void GLRenderer::handleAudioFrame(double timestamp, double sampleRate,
                                  juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer &midiBuffer)
{
    mutex.enter();

    midiFrames.emplace();
    midiFrames.back().timestamp = timestamp;
    for (auto metadata : midiBuffer) {
        const juce::MidiMessage &message = metadata.getMessage();
        midiFrames.back().buffer.addEvent(message, 0);
    }

    if (firstAudioTimestamp < 0.0) {
        firstAudioTimestamp = timestamp;

        if (sizeAudioChannel0 > 0) {
            sizeAudioChannel0 += (GLint)(sampleRate * DELAY_LATENCY);
            audioChannel0 = std::move(std::unique_ptr<float[]>(new float[sizeAudioChannel0]));
            memset(audioChannel0.get(), 0, sizeAudioChannel0 * sizeof(float));
        }

        if (sizeAudioChannel1 > 0) {
            sizeAudioChannel1 += (GLint)(sampleRate * DELAY_LATENCY);
            audioChannel1 = std::move(std::unique_ptr<float[]>(new float[sizeAudioChannel1]));
            memset(audioChannel1.get(), 0, sizeAudioChannel1 * sizeof(float));
        }
    }

    if (audioChannel0 != nullptr) {
        AdvanceAudioBuffer(audioChannel0.get(), sizeAudioChannel0,
                           buffer.getReadPointer(0),
                           buffer.getNumSamples());
    }

    if (audioChannel1 != nullptr) {
        AdvanceAudioBuffer(audioChannel1.get(), sizeAudioChannel1,
                           buffer.getReadPointer(1),
                           buffer.getNumSamples());
    }

    mSampleRate = sampleRate;

    mutex.exit();
}

void GLRenderer::resized()
{
    // Empty
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

    struct Intrinsic {
        const char *name;
        GLenum type;
        GLint sizeMin;
        GLint sizeMax;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> &uniform;
    };

    const Intrinsic intrinsics[] = {
        { "iResolution", GL_FLOAT_VEC2, 1, 1, program.resolutionIntrinsic },
        { "iKeyDown[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.keyDownIntrinsic },
        { "iKeyUp[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.keyUpIntrinsic },
        { "iTime", GL_FLOAT, 1, 1, program.timeIntrinsic },
        { "iSampleRate", GL_FLOAT, 1, 1, program.sampleRateIntrinsic },
        { "iAudioChannel0[0]", GL_FLOAT, 16, 2048, program.audioChannel0 },
        { "iAudioChannel1[0]", GL_FLOAT, 16, 2048, program.audioChannel1 }
    };

    for (int i = 0; i < sizeof(intrinsics) / sizeof(intrinsics[0]); i++) {
        if (name == intrinsics[i].name) {
            if (type != intrinsics[i].type || size < intrinsics[i].sizeMin || size > intrinsics[i].sizeMax) {
                goto failure;
            }

            intrinsics[i].uniform = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
                (new juce::OpenGLShaderProgram::Uniform(*program.program, intrinsics[i].name)));

            if (name == "iAudioChannel0[0]") {
                program.sizeAudioChannel0 = size;
                this->sizeAudioChannel0 = max(this->sizeAudioChannel0, program.sizeAudioChannel0);
            } else if (name == "iAudioChannel1[0]") {
                program.sizeAudioChannel1 = size;
                this->sizeAudioChannel1 = max(this->sizeAudioChannel1, program.sizeAudioChannel1);
            }

            isIntrinsic = true;
            return true;
        }
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
