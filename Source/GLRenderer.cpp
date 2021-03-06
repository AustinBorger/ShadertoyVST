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

GLRenderer::GLRenderer(ShadertoyAudioProcessor& processor,    // IN / OUT
                       ShadertoyAudioProcessorEditor &editor, // IN / OUT
                       juce::OpenGLContext &glContext)        // IN / OUT
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

void
GLRenderer::newOpenGLContextCreated()
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
    
    // Create the output framebuffer if needed
    if (!createFramebuffer(mOutputFramebuffer, 1)) {
        goto failure;
    }

    // Create the aux framebuffers if needed
    for (int i = 0; i < 4; i++) {
        if (!createFramebuffer(mAuxFramebuffers[i], 2 + i)) {
            goto failure;
        }
    }

    firstRender = -1.0;
    prevRender = -1.0;
    firstAudioTimestamp = -1.0;
    lastAudioTimestamp = -1.0;
    cacheLastAudioTimestamp = -1.0;

    for (int i = 0; i < MIDI_NUM_KEYS; i++) {
        keyDownLast[i] = -1.0f;
        keyUpLast[i] = -1.0f;
    }

    pitchWheel = 0.0f;
    sustainPedal = 0.0f;
    sostenutoPedal = 0.0f;
    softPedal = 0.0f;

#if GLRENDER_LOG_FPS == 1
    avgFPS = 0.0;
    lastFPSLog = 0.0;
#endif

    processor.addAudioListener(this);
    
    validState = true;
    return;
    
failure:
    validState = false;
}

void
GLRenderer::openGLContextClosing()
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
    maxSizeAudioChannel0 = 0;
    audioChannel1 = nullptr;
    sizeAudioChannel1 = 0;
    maxSizeAudioChannel0 = 0;

    cacheAudioChannel0 = nullptr;
    cacheSizeAudioChannel0 = 0;
    cacheAudioChannel1 = nullptr;
    cacheSizeAudioChannel1 = 0;
}

void
GLRenderer::setProgramIntrinsics(int programIdx,               // IN
                                 double currentAudioTimestamp, // IN
                                 int backBufferWidth,          // IN
                                 int backBufferHeight)         // IN
{
    ProgramData &program = programData[programIdx];

    /*
     * Calculate the difference between simulated audio time and the
     * audio timestamp last provided by handleAudioFrame. Use this to
     * advance the audio buffer given to the shader at a constant rate.
     * If this is not done, there will be variable jumps in audio because
     * handleAudioFrame is called at unpredictable time intervals.
     */
    double audioTimeDiff = currentAudioTimestamp - cacheLastAudioTimestamp;
    int samplePos = max(0, min(int(mSampleRate * (audioTimeDiff + DELAY_LATENCY)),
                               int(mSampleRate * DELAY_LATENCY)));

    if (program.audioChannel0 != nullptr && cacheAudioChannel0 != nullptr) {
        GLint sizeDiff = maxSizeAudioChannel0 - program.sizeAudioChannel0;
        program.audioChannel0->set(cacheAudioChannel0.get() + sizeDiff + samplePos,
                                   program.sizeAudioChannel0);
    }

    if (program.audioChannel1 != nullptr && cacheAudioChannel1 != nullptr) {
        GLint sizeDiff = maxSizeAudioChannel1 - program.sizeAudioChannel1;
        program.audioChannel1->set(cacheAudioChannel1.get() + sizeDiff + samplePos,
                                   program.sizeAudioChannel1);
    }

    if (program.sampleRateIntrinsic != nullptr) {
        program.sampleRateIntrinsic->set((GLfloat)mSampleRate);
    }

    for (auto it = program.uniformFloats.begin(); it != program.uniformFloats.end(); ++it) {
        int uniformIdx = it->first;
        float val = processor.getUniformFloat(uniformIdx);
        it->second->set(val);
    }

    for (auto it = program.uniformInts.begin(); it != program.uniformInts.end(); ++it) {
        int uniformIdx = it->first;
        int val = processor.getUniformInt(uniformIdx);
        it->second->set(val);
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

    if (program.keyDownVelocityIntrinsic != nullptr) {
        GLfloat vals[MIDI_NUM_KEYS] = { };
        for (int i = 0; i < MIDI_NUM_KEYS; i++) {
            vals[i] = (GLfloat)keyDownVelocity[i];
        }
        program.keyDownVelocityIntrinsic->set(vals, MIDI_NUM_KEYS);
    }

    if (program.keyUpVelocityIntrinsic != nullptr) {
        GLfloat vals[MIDI_NUM_KEYS] = { };
        for (int i = 0; i < MIDI_NUM_KEYS; i++) {
            vals[i] = (GLfloat)keyUpVelocity[i];
        }
        program.keyUpVelocityIntrinsic->set(vals, MIDI_NUM_KEYS);
    }

    if (program.afterTouchIntrinsic != nullptr) {
        GLfloat vals[MIDI_NUM_KEYS] = { };
        for (int i = 0; i < MIDI_NUM_KEYS; i++) {
            vals[i] = (GLfloat)afterTouch[i];
        }
        program.afterTouchIntrinsic->set(vals, MIDI_NUM_KEYS);
    }

    if (program.pitchWheelIntrinsic != nullptr) {
        program.pitchWheelIntrinsic->set((GLfloat)pitchWheel);
    }

    if (program.sustainPedalIntrinsic != nullptr) {
        program.sustainPedalIntrinsic->set((GLfloat)sustainPedal);
    }

    if (program.sostenutoPedalIntrinsic != nullptr) {
        program.sostenutoPedalIntrinsic->set((GLfloat)sostenutoPedal);
    }

    if (program.softPedalIntrinsic != nullptr) {
        program.softPedalIntrinsic->set((GLfloat)softPedal);
    }

    if (program.channelPressureIntrinsic != nullptr) {
        program.channelPressureIntrinsic->set((GLfloat)channelPressure);
    }

    if (program.timeIntrinsic != nullptr) {
        program.timeIntrinsic->set((GLfloat)currentAudioTimestamp);
    }

    int outputFbWidth = 0, outputFbHeight = 0;
    int auxFbWidth[4] = { }, auxFbHeight[4] = { };

    int outputProgramIdx = processor.getOutputProgramIdx();
    if (processor.getShaderDestination(outputProgramIdx) == 1) {
        if (processor.getShaderFixedSizeBuffer(outputProgramIdx)) {
            outputFbWidth = processor.getShaderFixedSizeWidth(outputProgramIdx);
            outputFbHeight = processor.getShaderFixedSizeHeight(outputProgramIdx);
        } else {
            outputFbWidth = backBufferWidth;
            outputFbHeight = backBufferHeight;
        }
    }

    for (int i = 0; i < 4; i++) {
        int bufferProgramIdx = processor.getBufferProgramIdx(i);
        if (processor.getShaderDestination(bufferProgramIdx) == i + 2) {
            if (processor.getShaderFixedSizeBuffer(bufferProgramIdx)) {
                auxFbWidth[i] = processor.getShaderFixedSizeWidth(bufferProgramIdx);
                auxFbHeight[i] = processor.getShaderFixedSizeHeight(bufferProgramIdx);
            } else {
                auxFbWidth[i] = mAuxFramebuffers[i].width;
                auxFbHeight[i] = mAuxFramebuffers[i].height;
            }
        }
    }

    if (program.outputResolutionIntrinsic != nullptr) {
        program.outputResolutionIntrinsic->set((GLfloat)outputFbWidth,
                                               (GLfloat)outputFbHeight);
    }

    for (int i = 0; i < 4; i++) {
        if (program.auxResolutionIntrinsic[i] != nullptr) {
            program.auxResolutionIntrinsic[i]->set((GLfloat)auxFbWidth[i],
                                                   (GLfloat)auxFbHeight[i]);
        }

        if (program.auxBufferIntrinsic[i] != nullptr &&
            processor.getShaderDestination(programIdx) != 2 + i) {
            glContext.extensions.glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, mAuxFramebuffers[i].textureObj);
            program.auxBufferIntrinsic[i]->set(i);
        }
    }
}

void
GLRenderer::renderAuxBuffer(int bufferIdx,                // IN
                            double currentAudioTimestamp, // IN
                            int backBufferWidth,          // IN
                            int backBufferHeight)         // IN
{
    int programIdx = processor.getBufferProgramIdx(bufferIdx);
    if (programIdx < programData.size() &&
        processor.getShaderDestination(programIdx) == 2 + bufferIdx) {
        ProgramData &program = programData[programIdx];
        program.program->use();

        setProgramIntrinsics(programIdx, currentAudioTimestamp,
                             backBufferWidth, backBufferHeight);

        glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, mAuxFramebuffers[bufferIdx].framebufferObj);
        if (processor.getShaderFixedSizeBuffer(programIdx)) {
            glViewport(0, 0, processor.getShaderFixedSizeWidth(programIdx),
                       processor.getShaderFixedSizeHeight(programIdx));
        } else {
            glViewport(0, 0, mAuxFramebuffers[bufferIdx].width,
                       mAuxFramebuffers[bufferIdx].height);
        }

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

void
GLRenderer::renderOutputBuffer(double currentAudioTimestamp, // IN
                               int backBufferWidth,          // IN
                               int backBufferHeight)         // IN
{
    int programIdx = processor.getOutputProgramIdx();
    if (programIdx < programData.size() &&
        processor.getShaderDestination(programIdx) == 1) {
        ProgramData &program = programData[programIdx];
        program.program->use();

        setProgramIntrinsics(programIdx, currentAudioTimestamp,
                             backBufferWidth, backBufferHeight);
        
        if (processor.getShaderFixedSizeBuffer(programIdx)) {
            int framebufferWidth = processor.getShaderFixedSizeWidth(programIdx);
            int framebufferHeight = processor.getShaderFixedSizeHeight(programIdx);

            /*
             * First draw to fixed-size framebuffer
             */
            glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, mOutputFramebuffer.framebufferObj);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glDrawArrays(GL_TRIANGLES, 0, 3);
    
            /*
             * Now stretch to the render area
             */
            copyProgram.use();

            widthRatio->set((float)mOutputFramebuffer.width / (float)framebufferWidth);
            heightRatio->set((float)mOutputFramebuffer.height / (float)framebufferHeight);
            
            glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glContext.extensions.glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mOutputFramebuffer.textureObj);
            glViewport(0, 0, backBufferWidth, backBufferHeight);
        } else {
            /*
             * Draw directly to back buffer
             */
            glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, backBufferWidth, backBufferHeight);
        }

        glDrawArrays(GL_TRIANGLES, 0, 3);
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

void
GLRenderer::renderOpenGL()
{
    jassert(juce::OpenGLHelpers::isContextActive());
    juce::OpenGLHelpers::clear(juce::Colours::black);

    if (validState) {
        double scaleFactor = glContext.getRenderingScale(); // DPI scaling
        int backBufferWidth = (int)(getWidth() * scaleFactor);
        int backBufferHeight = (int)(getHeight() * scaleFactor);
        double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        double elapsedSeconds = -1.0;
        double currentAudioTimestamp = -1.0;

        mutex.enter();

#if GLRENDER_LOG_FPS == 1
        if (prevRender >= 0.0) {
            double instFPS = 1.0 / (now - prevRender);
            if (avgFPS > 0.0) {
                const double a = 0.96;
                avgFPS = avgFPS * a + instFPS * (1.0 - a);
            }  else {
                avgFPS = instFPS;
            }
        }

        if (avgFPS > 0.0) {
            if (now - lastFPSLog > 1.0) {
                editor.logDebugMessage("FPS: " + std::to_string(avgFPS));
                lastFPSLog = now;
            }
        }
#endif

        if (firstRender < 0) {
            firstRender = now;
        }
        elapsedSeconds = now - firstRender;

        if (firstAudioTimestamp >= 0.0) {
            currentAudioTimestamp = min(lastAudioTimestamp,
                                        max(firstAudioTimestamp - DELAY_LATENCY + elapsedSeconds,
                                            lastAudioTimestamp - DELAY_LATENCY));
            while (!midiFrames.empty() && midiFrames.front().timestamp <= currentAudioTimestamp) {
                MidiFrame &midiFrame = midiFrames.front();
                for (auto metadata : midiFrame.buffer) {
                    const juce::MidiMessage &message = metadata.getMessage();
                    if (message.isNoteOn()) {
                        keyDownLast[message.getNoteNumber()] = midiFrame.timestamp;
                        keyDownVelocity[message.getNoteNumber()] = message.getFloatVelocity();
                    } else if (message.isNoteOff()) {
                        keyUpLast[message.getNoteNumber()] = midiFrame.timestamp;
                        keyUpVelocity[message.getNoteNumber()] = message.getFloatVelocity();
                    } else if (message.isAftertouch()) {
                        afterTouch[message.getNoteNumber()] = (float)(message.getAfterTouchValue()) / 127;
                    } else if (message.isPitchWheel()) {
                        pitchWheel = (float)(message.getPitchWheelValue()) / int(0x3fff);
                    } else if (message.isSustainPedalOn()) {
                        sustainPedal = 1.0f;
                    } else if (message.isSustainPedalOff()) {
                        sustainPedal = 0.0f;
                    } else if (message.isSostenutoPedalOn()) {
                        sostenutoPedal = 1.0f;
                    } else if (message.isSostenutoPedalOff()) {
                        sostenutoPedal = 0.0f;
                    } else if (message.isSoftPedalOn()) {
                        softPedal = 1.0f;
                    } else if (message.isSoftPedalOff()) {
                        softPedal = 0.0f;
                    } else if (message.isChannelPressure()) {
                        channelPressure = (float)(message.getChannelPressureValue()) / 127;
                    }
                }
                midiFrames.pop();
            }
        }

        if (cacheSizeAudioChannel0 != sizeAudioChannel0) {
            cacheAudioChannel0 = nullptr;
            if (sizeAudioChannel0 > 0) {
                cacheAudioChannel0 = std::move(std::unique_ptr<float[]>(new float[sizeAudioChannel0]));
            }
            cacheSizeAudioChannel0 = sizeAudioChannel0;
        }

        if (cacheSizeAudioChannel1 != sizeAudioChannel1) {
            cacheAudioChannel1 = nullptr;
            if (sizeAudioChannel1 > 0) {
                cacheAudioChannel1 = std::move(std::unique_ptr<float[]>(new float[sizeAudioChannel1]));
            }
            cacheSizeAudioChannel1 = sizeAudioChannel1;
        }

        if (audioChannel0 != nullptr && cacheAudioChannel0 != nullptr) {
            memcpy(cacheAudioChannel0.get(), audioChannel0.get(),
                   sizeof(float) * sizeAudioChannel0);
        }

        if (audioChannel1 != nullptr && cacheAudioChannel1 != nullptr) {
            memcpy(cacheAudioChannel1.get(), audioChannel1.get(),
                   sizeof(float) * sizeAudioChannel1);
        }

        cacheLastAudioTimestamp = lastAudioTimestamp;

        mutex.exit();

        for (int i = 0; i < 4; i++) {
            renderAuxBuffer(i, currentAudioTimestamp, backBufferWidth, backBufferHeight);
        }
        
        renderOutputBuffer(currentAudioTimestamp, backBufferWidth, backBufferHeight);

        prevRender = now;
    }
}

/*
 * AdvanceAudioBuffer
 *    Updates the audio buffer with more current samples.
 */
static void
AdvanceAudioBuffer(float *dst,       // OUT: The history buffer
                   int dstSize,      // IN: The size of the history buffer
                   const float *src, // IN: The buffer filled with new samples
                   int srcSize)      // IN: The number of new samples
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
void
GLRenderer::handleAudioFrame(double timestamp,                 // IN
                             double sampleRate,                // IN
                             juce::AudioBuffer<float>& buffer, // IN
                             juce::MidiBuffer &midiBuffer)     // IN
{
    mutex.enter();

    if (firstAudioTimestamp < 0.0 || mSampleRate != sampleRate) {
        firstAudioTimestamp = timestamp;
        firstRender = -1.0;
        prevRender = -1.0;
        midiFrames = { };

        if (maxSizeAudioChannel0 > 0) {
            sizeAudioChannel0 = maxSizeAudioChannel0 + (GLint)(sampleRate * DELAY_LATENCY);
            audioChannel0 = std::move(std::unique_ptr<float[]>(new float[sizeAudioChannel0]));
            memset(audioChannel0.get(), 0, sizeAudioChannel0 * sizeof(float));
        }

        if (maxSizeAudioChannel1 > 0) {
            sizeAudioChannel1 = maxSizeAudioChannel1 + (GLint)(sampleRate * DELAY_LATENCY);
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

    midiFrames.emplace();
    midiFrames.back().timestamp = timestamp;
    for (auto metadata : midiBuffer) {
        const juce::MidiMessage &message = metadata.getMessage();
        midiFrames.back().buffer.addEvent(message, 0);
    }

    mSampleRate = sampleRate;
    lastAudioTimestamp = timestamp;

    mutex.exit();
}

void
GLRenderer::resized()
{
    // Empty
}

bool
GLRenderer::loadExtensions()
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

bool
GLRenderer::checkIntrinsicUniform(const juce::String &name, // IN
                                  GLenum type,              // IN
                                  GLint size,               // IN
                                  bool &isIntrinsic,        // OUT
                                  int programIdx)           // IN
{
    ProgramData &program = programData[programIdx];
    juce::String failReason;
    isIntrinsic = false;

    struct Intrinsic {
        const char *name;
        GLenum type;
        GLint sizeMin;
        GLint sizeMax;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> &uniform;
    };

    const Intrinsic intrinsics[] = {
        { "iResolution", GL_FLOAT_VEC2, 1, 1, program.outputResolutionIntrinsic },
        { "iResolutionBufferA", GL_FLOAT_VEC2, 1, 1, program.auxResolutionIntrinsic[0] },
        { "iResolutionBufferB", GL_FLOAT_VEC2, 1, 1, program.auxResolutionIntrinsic[1] },
        { "iResolutionBufferC", GL_FLOAT_VEC2, 1, 1, program.auxResolutionIntrinsic[2] },
        { "iResolutionBufferD", GL_FLOAT_VEC2, 1, 1, program.auxResolutionIntrinsic[3] },
        { "iBufferA", GL_SAMPLER_2D, 1, 1, program.auxBufferIntrinsic[0] },
        { "iBufferB", GL_SAMPLER_2D, 1, 1, program.auxBufferIntrinsic[1] },
        { "iBufferC", GL_SAMPLER_2D, 1, 1, program.auxBufferIntrinsic[2] },
        { "iBufferD", GL_SAMPLER_2D, 1, 1, program.auxBufferIntrinsic[3] },
        { "iKeyDown[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.keyDownIntrinsic },
        { "iKeyUp[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.keyUpIntrinsic },
        { "iKeyDownVelocity[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.keyDownVelocityIntrinsic },
        { "iKeyUpVelocity[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.keyUpVelocityIntrinsic },
        { "iAfterTouch[0]", GL_FLOAT, MIDI_NUM_KEYS, MIDI_NUM_KEYS, program.afterTouchIntrinsic },
        { "iPitchWheel", GL_FLOAT, 1, 1, program.pitchWheelIntrinsic },
        { "iSustainPedal", GL_FLOAT, 1, 1, program.sustainPedalIntrinsic },
        { "iSostenutoPedal", GL_FLOAT, 1, 1, program.sostenutoPedalIntrinsic },
        { "iSoftPedal", GL_FLOAT, 1, 1, program.softPedalIntrinsic },
        { "iChannelPressure", GL_FLOAT, 1, 1, program.channelPressureIntrinsic },
        { "iTime", GL_FLOAT, 1, 1, program.timeIntrinsic },
        { "iSampleRate", GL_FLOAT, 1, 1, program.sampleRateIntrinsic },
        { "iAudioChannel0[0]", GL_FLOAT, 16, 2048, program.audioChannel0 },
        { "iAudioChannel1[0]", GL_FLOAT, 16, 2048, program.audioChannel1 }
    };

    for (int i = 0; i < sizeof(intrinsics) / sizeof(intrinsics[0]); i++) {
        if (name == intrinsics[i].name) {
            if (type != intrinsics[i].type) {
                failReason = "Incorrect type";
                goto failure;
            }
            
            if (size < intrinsics[i].sizeMin || size > intrinsics[i].sizeMax) {
                failReason = "Incorrect size";
                goto failure;
            }

            for (int j = 0; j < 4; j++) {
                juce::String bufferName = "iBuffer";
                bufferName += char('A' + j);
                if (name == bufferName && processor.getShaderDestination(programIdx) == 2 + j) {
                    failReason = "Cannot use the output buffer as an input sampler2D";
                    goto failure;
                }
            }

            intrinsics[i].uniform = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
                (new juce::OpenGLShaderProgram::Uniform(*program.program, intrinsics[i].name)));

            if (name == "iAudioChannel0[0]") {
                program.sizeAudioChannel0 = size;
                maxSizeAudioChannel0 = max(maxSizeAudioChannel0, size);
            } else if (name == "iAudioChannel1[0]") {
                program.sizeAudioChannel1 = size;
                maxSizeAudioChannel1 = max(maxSizeAudioChannel1, size);
            }

            isIntrinsic = true;
            return true;
        }
    }
    
    return true;
    
failure:
    alertError("Error reading uniforms for program " + std::to_string(programIdx),
               "Illegal use of intrinsic uniform name \"" + name + "\", reason: " + failReason);
    return false;
}

bool isDigit(const juce::String &str)
{
    for (int i = 0; i < str.length(); i++) {
        if (str[i] < '0' || str[i] > '9') {
            return false;
        }
    }

    return true;
}

bool
GLRenderer::buildShaderProgram(int idx) // IN
{
    ProgramData &program = programData[idx];
    juce::String message;

    if (!program.program->addVertexShader(vert) ||
	    !program.program->addFragmentShader(processor.getShaderString(idx)) ||
	    !program.program->link()) {
	    alertError("Error building program " + std::to_string(idx),
                   program.program->getLastError());
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
            int uniformIdx = 0;

            if (size != 1) {
	            message = "Parameter uniform \"";
	            message += name;
	            message += "\" cannot be an array.";
	            goto failure;
	        }

            if (type == GL_FLOAT) {
                if (nameStr.substring(0, 5) != "float" ||
                    !isDigit(nameStr.substring(5)) ||
                    (uniformIdx = nameStr.substring(5).getIntValue()) < 0 ||
                    uniformIdx > 255) {
                    message = "Parameter uniform \"";
                    message += name;
                    message += "\" must be named float0..255";
                    goto failure;
                }

                program.uniformFloats[uniformIdx] = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	                (new juce::OpenGLShaderProgram::Uniform(*program.program, name)));
            } else if (type == GL_INT) {
                if (nameStr.substring(0, 3) != "int" ||
                    !isDigit(nameStr.substring(3)) ||
                    (uniformIdx = nameStr.substring(3).getIntValue()) < 0 ||
                    uniformIdx > 255) {
                    message = "Parameter uniform \"";
                    message += name;
                    message += "\" must be named int0..255";
                    goto failure;
                }

                program.uniformInts[uniformIdx] = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	                (new juce::OpenGLShaderProgram::Uniform(*program.program, name)));
            } else {
	            message = "Parameter uniform \"";
	            message += name;
	            message += "\" of unrecognized type.";
	            goto failure;
	        }
	    }
	}
	
	return true;

failure:
    alertError("Error reading uniforms for program " + std::to_string(idx), message);
    return false;
}

bool
GLRenderer::buildCopyProgram()
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

bool
GLRenderer::createFramebuffer(Framebuffer &fbOut, // OUT
                              int destinationId)  // IN
{
    // Figure out if we have to create one and how big it needs to be
    bool shouldCreateBuffer = false;
    fbOut.width = 640;
    fbOut.height = 360;

    for (int i = 0; i < processor.getNumShaderFiles(); i++) {
        if (processor.getShaderDestination(i) == destinationId) {
            shouldCreateBuffer = true;
            if (processor.getShaderFixedSizeBuffer(i)) {
                fbOut.width = max(fbOut.width, processor.getShaderFixedSizeWidth(i));
                fbOut.height = max(fbOut.height, processor.getShaderFixedSizeHeight(i));
            } else {
                fbOut.width = max(fbOut.width, processor.getVisualizationWidth());
                fbOut.height = max(fbOut.height, processor.getVisualizationHeight());
            }
        }
    }
    
    if (!shouldCreateBuffer) {
        return true;
    }
    
    glContext.extensions.glGenFramebuffers(1, &fbOut.framebufferObj);
    glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, fbOut.framebufferObj);
    glGenTextures(1, &fbOut.textureObj);
    glBindTexture(GL_TEXTURE_2D, fbOut.textureObj);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbOut.width, fbOut.height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glContext.extensions.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                                fbOut.textureObj, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    
    if (glContext.extensions.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        alertError("Unable to construct framebuffer", "Failed to construct the framebuffer");
        return false;
    }

    return true;
}

GLRenderer::Framebuffer &
GLRenderer::destinationToFramebuffer(int destinationId) // IN
{
    if (destinationId == 1) {
        return mOutputFramebuffer;
    } else {
        return mAuxFramebuffers[destinationId - 2];
    }
}

void
GLRenderer::alertError(const juce::String &title,   // IN
                       const juce::String &message) // IN
{
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
	                                       title, message);
}
