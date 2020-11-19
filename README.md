## Overview

ShadertoyVST is an adaptation of the [Shadertoy](https://www.shadertoy.com/)
development environment in the form of a VST plugin. Its purpose is for creating
music visualizations, and therefore implements features that are more suited to
those ends. Automatable parameters and MIDI events are forwarded to the shaders
as uniforms, facilitating the use of a DAW to produce rhythmically guided animations.

## Installation

Download the latest release [here](https://github.com/AustinBorger/ShadertoyVST/releases/download/20201118/Shadertoy.vst3),
and copy the .vst3 to C:\Program Files\Common Files\VST3 or your preferred VST3 installation location.

## Getting Started

For an introduction, see the [Getting Started Guide](Documentation/GETTINGSTARTED.md).

## Supported Platforms

ShadertoyVST makes use of JUCE which is a cross-platform, cross-DAW framework for
creating audio plugins. Currently, the only tested platform / DAW combination is
FL Studio 20 on Windows 10. Bugs are likely to exist on other Operating Systems
and DAWs. See the Contribution section if you would like to help improve stability
on these untested environments.

## Contribution

All contributors are welcome. This project is in need of contributors on other
platforms to verify correct behavior of the VST plugin.

If you have a feature you would like to add to ShadertoyVST, fork off a new
repo and submit a pull request.

Bug reports should be submitted through the normal GitHub issue tracker.

Core Contributors:
- Austin Borger (aaborger@gmail.com)

## License

ShadertoyVST uses JUCE, which is primarily licensed under the 
[GPL/Commercial license](https://www.gnu.org/licenses/gpl-3.0.en.html).
As a consequence, the same license applies to ShadertoyVST.