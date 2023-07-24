# OculusDirectDisplayAdditions
3rd-party "DirectDisplay" implementation which adds an "extended display" mode for the Oculus PC app, intended for Intel (and NVIDIA Optimus with HDMI wired to the Intel GPU) graphics.

This is only works for ***tethered*** Oculus headsets, such as the Rift S or CV1. It is useless for PCs (usually desktops and certain laptops) which have the HDMI/DisplayPort port wired to the NVIDIA GPU, or headsets which use Quest Link or Air Link.

# Where are builds at?
Support me on [Patreon](https://patreon.com/join/itskaitlyn03) to get access to release builds of this project (for the time being), the source code is provided as a courtesy and you will not obtain support from me if you build it yourself and run into issues.

Once this project is ready for public consumption, this barrier will be lifted.

# Notes
As this project largely consists of code that was reverse-engineered by myself over the span of a month (basically), there could be issues that can break OVRServer into a state which ***might*** require a system restart. I am not responsible for any hardware or system damage caused by this project, although it is very unlikely to cause any damage to either at all.

If you experience any crashes you think were caused by this project, please create an issue and describe to the best of your ability how to reproduce it (if possible) and if/any crash logs.

# How does this work?
As it turns out, OVRServer will actually check for 3rd-party DirectDisplay implementations on startup, as long as `HKEY_LOCAL_MACHINE\SOFTWARE\Oculus\DirectDisplayDLL` is a path to a DLL library which contains a few exports, etc. This is an undocumented feature in OVRServer.

This allows us to provide "DirectDisplay" support for GPUs other than AMD or NVIDIA, such as NVIDIA Optimus (HDMI wired to the Intel GPU, such as most laptops) or even Intel Arc (at some point). Thus, allowing you to use your tethered Oculus headsets on your PC with those unsupported GPUs.
