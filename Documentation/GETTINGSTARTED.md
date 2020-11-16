## GUI

![](gui.PNG)

The GUI for ShadertoyVST is pretty simple at the moment. The VST window is
split up into two tabs - the patch editor, and the visualization output.

The patch editor is split up into three sections:
- On the left-hand side you have the list of shaders. Each shader is assigned
an ID and a file location. Buttons on the bottom allow you to load, delete,
and reload shaders to the list.
- In the top-right you have shader-specific properties, such as which output
framebuffer the shader renders to and, if desired, a fixed width and height
for the output.
- In the bottom-right you have global properties. Right now this is just the
size of the visualization output.

## Parameters

![](parameters.PNG)

The list of parameters is as such:

- The program IDs of the active shaders for each of the output and auxiliary
buffers (BufferA..D)
- Floating point parameters between 0..1
- Integer parameters between 0..100