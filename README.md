# OculusIntelDirectDisplay
3rd-party "DirectDisplay" implementation for Intel (and NVIDIA Optimus) graphics, for Oculus PC app.

This is only intended for ***tethered*** Oculus headsets, such as the Rift S or CV1. It is useless for laptops which have the HDMI/DisplayPort port wired to the NVIDIA GPU, or headsets which use Quest Link or Air Link.

# How does this work?
As it turns out, OVRServer will actually check for 3rd-party DirectDisplay implementations on startup, as long as `HKEY_LOCAL_MACHINE\SOFTWARE\Oculus\DirectDisplayDLL` is a path to a DLL library which contains a few exports, etc. This is an undocumented feature in OVRServer.

This allows us to provide "DirectDisplay" support for GPUs other than AMD or NVIDIA, such as NVIDIA Optimus or even Intel Arc. Thus, allowing you to use your tethered Oculus headsets on your PC with those unsupported GPUs.
