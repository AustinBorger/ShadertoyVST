/*
  ==============================================================================

    GLRenderer.cpp
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin

  ==============================================================================
*/

#include <JuceHeader.h>
#include "GLRenderer.h"

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
"\n"
"void main() {\n"
    "FragColor = texture(visuTexture, texCoord);\n"
"}\n";

static const juce::String frag =
"#version 330\n"
"out vec4 FragColor;\n"
"uniform float red;\n"
"uniform float green;\n"
"uniform float blue;\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vec4(red, green, blue, 1.0);\n"
"}\n";

static const juce::String frag2 =
"#version 330\n"
"out vec4 fragColor;\n"
"in vec2 texCoord;\n"
"uniform vec2 iResolution;\n"
"uniform float iTime;\n"
"#define PI 3.14159265359\n"
"\n"
"vec3 col1 = vec3(0.216, 0.471, 0.698);\n" // blue
"vec3 col2 = vec3(1.00, 0.329, 0.298);\n" // yellow
"vec3 col3 = vec3(0.867, 0.910, 0.247);\n" // red
"\n"
"float disk(vec2 r, vec2 center, float radius) {\n"
"	return 1.0 - smoothstep( radius-0.008, radius+0.008, length(r-center));\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	float t = iTime*2.;\n"
"   vec2 fragCoord = texCoord.xy * iResolution.xy;\n"
"	vec2 r = (2.0*fragCoord.xy - iResolution.xy) / iResolution.y;\n"
"	r *= 1.0 + 0.05*sin(r.x*5.+iTime) + 0.05*sin(r.y*3.+iTime);\n"
"	r *= 1.0 + 0.2*length(r);\n"
"	float side = 0.5;\n"
"	vec2 r2 = mod(r, side);\n"
"	vec2 r3 = r2-side/2.;\n"
"	float i = floor(r.x/side)+2.;\n"
"	float j = floor(r.y/side)+4.;\n"
"	float ii = r.x/side+2.;\n"
"	float jj = r.y/side+4.;\n"	
"\n"
"	vec3 pix = vec3(1.0);\n"
"\n"
"	float rad, disks;\n"
"\n"
"	rad = 0.15 + 0.05*sin(t+ii*jj);\n"
"	disks = disk(r3, vec2(0.,0.), rad);\n"
"	pix = mix(pix, col2, disks);\n"
"\n"
"	float speed = 2.0;\n"
"	float tt = iTime*speed+0.1*i+0.08*j;\n"
"	float stopEveryAngle = PI/2.0;\n"
"	float stopRatio = 0.7;\n"
"	float t1 = (floor(tt) + smoothstep(0.0, 1.0-stopRatio, fract(tt)) )*stopEveryAngle;\n"
"\n"
"	float x = -0.07*cos(t1+i);\n"
"	float y = 0.055*(sin(t1+j)+cos(t1+i));\n"
"	rad = 0.1 + 0.05*sin(t+i+j);\n"
"	disks = disk(r3, vec2(x,y), rad);\n"
"	pix = mix(pix, col1, disks);\n"
"\n"
"	rad = 0.2 + 0.05*sin(t*(1.0+0.01*i));\n"
"	disks = disk(r3, vec2(0.,0.), rad);\n"
"	pix += 0.2*col3*disks * sin(t+i*j+i);\n"
"\n"
"	pix -= smoothstep(0.3, 5.5, length(r));\n"	
"	fragColor = vec4(pix,1.0);\n"
"}\n";

//==============================================================================
GLRenderer::GLRenderer(ShadertoyAudioProcessor& audioProcessor,
                       juce::OpenGLContext &glContext)
 : audioProcessor(audioProcessor),
   glContext(glContext),
   program(glContext),
   copyProgram(glContext),
   validState(true),
   uniforms()
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

    if (!buildShaderProgram()) {
        goto failure;
    }
    
    if (!buildCopyProgram()) {
        goto failure;
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
    resolutionIntrinsic = nullptr;
    uniforms.clear();
    program.release();
    copyProgram.release();
}

void GLRenderer::renderOpenGL()
{
    jassert(juce::OpenGLHelpers::isContextActive());
    juce::OpenGLHelpers::clear(juce::Colours::black);

    if (validState) {
        double scaleFactor = glContext.getRenderingScale(); // DPI scaling

        /*
         * First draw to fixed-size framebuffer
         */
        glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
        glBindTexture(GL_TEXTURE_2D, mRenderTexture);
        glViewport(0, 0, VISU_WIDTH, VISU_HEIGHT);
        program.use();
        
        if (resolutionIntrinsic != nullptr) {
            resolutionIntrinsic->set(VISU_WIDTH, VISU_HEIGHT);
        }
        
        for (int i = 0; i < uniforms.size(); i++) {
            float val = audioProcessor.getUniformFloat(i);
            uniforms[i]->set(val);
        }
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        /*
         * Now stretch to the render area
         */
        glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, getWidth() * scaleFactor, getHeight() * scaleFactor);
        copyProgram.use();
        glDrawArrays(GL_TRIANGLES, 0, 3);
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
                                       bool &isIntrinsic)
{
    isIntrinsic = false;
    
    static const char *RESOLUTION_INTRINSIC_NAME = "iResolution";

    if (name == RESOLUTION_INTRINSIC_NAME) {
        if (type != GL_FLOAT_VEC2 || size != 1) {
            goto failure;
        }
        
        resolutionIntrinsic = std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
            (new juce::OpenGLShaderProgram::Uniform(program, RESOLUTION_INTRINSIC_NAME)));
        
        isIntrinsic = true;
        return true;
    }
    
    return true;
    
failure:
    alertError("Error reading uniforms",
               "Illegal use of intrinsic uniform name \"" + name + "\"");
    return false;
}

bool GLRenderer::buildShaderProgram()
{
    if (!program.addVertexShader(vert) ||
	    !program.addFragmentShader(frag2) ||
	    !program.link()) {
	    alertError("Error building program", program.getLastError());
	    return false;
	}
	
	GLint count;
	glContext.extensions.glGetProgramiv(program.getProgramID(), GL_ACTIVE_UNIFORMS, &count);
	
	GLchar name[256];
	GLsizei length;
	GLint size;
	GLenum type;
	for (GLint i = 0; i < count; i++) {
	    glGetActiveUniform(program.getProgramID(), (GLuint)i, ARRAYSIZE(name),
	                       &length, &size, &type, name);
	    name[length] = '\0';

	    const juce::String nameStr = name;
	    bool isIntrinsic;
	    if (!checkIntrinsicUniform(nameStr, type, size, isIntrinsic)) {
	        return false;
	    }
	    
	    if (!isIntrinsic) {
	        if (type != GL_FLOAT) {
	            juce::String message = "Parameter uniform \"";
	            message += name;
	            message += "\" must be a float.";
	            alertError("Error reading uniforms", message);
	            return false;
	        } else if (size != 1) {
	            juce::String message = "Parameter uniform \"";
	            message += name;
	            message += "\" cannot be an array.";
	            alertError("Error reading uniforms", message);
	            return false;
	        }
	    
	        uniforms.emplace_back(std::move(std::unique_ptr<juce::OpenGLShaderProgram::Uniform>
	            (new juce::OpenGLShaderProgram::Uniform(program, name))));
	    }
	}
	
	return true;
}

bool GLRenderer::buildCopyProgram()
{
    if (!copyProgram.addVertexShader(vert) ||
        !copyProgram.addFragmentShader(copyFrag) ||
        !copyProgram.link()) {
        alertError("Error building copy program", copyProgram.getLastError());
        return false;
    }

    return true;
}

bool GLRenderer::createFramebuffer()
{
    glContext.extensions.glGenFramebuffers(1, &mFramebuffer);
    glContext.extensions.glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glGenTextures(1, &mRenderTexture);
    glBindTexture(GL_TEXTURE_2D, mRenderTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VISU_WIDTH, VISU_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
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
