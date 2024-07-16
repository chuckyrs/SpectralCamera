# SpectralCamera
A low-power hyperspectral camera based around a Jetson Nano, Pi Cam and AS7265x spectral sensor.

This code is central to the work done by Sutherland et al. (2024) in the paper: Sutherland, C.; Henderson, A.D.; Giosio, D.R.; Trotter, A.J.; Smith, G.G. Sensing Pre- and Post-Ecdysis of Tropical Rock Lobsters, *Panulirus ornatus*, Using a Low-Cost Novel Spectral Camera. J. Mar. Sci. Eng. 2024, 12, 987. https://doi.org/ 10.3390/jmse12060987

Two further articles are being prepared about the low-cost spectral camera. One in HardwareX called "Synchronising an IMX219 image sensor and AS7265x spectral sensor to make a novel low-cost spectral camera." and one in SoftwareX "Nvgstcapture-1.0 Modified for Autofocus and Peripheral Hardware Interfaces."

The motivation for this project arose because I needed software that could control a camera, lights and the spectral sensor, preferrably from the same board. The Jetson Nano with nvgstcapture-1.0 provides a very responsive on screen preview, with simple keystroke image capture. The Jetson also defaults to using the Raspberry Pi Camera Module V2, which has the Sony IMX219 image sensor. I needed to do short distance imaging, and I needed auto focus. Arducam make an Autofocus IMX219 in Visible light and NOIR varients. It seems that the NOIR versions are now only available as modules, but the visible light IMX219 AF image sensors are still available on carrier boards.

Sampling the lobsters meant one hand was busy aligning the camera via the on screen preview, and the other hand was wet with salty seawater. No good for electronics. A remote shutter release was required as well.

This software has C++ classes that interact with the GPIO pins and a USB UART device, as well as i2C for focussing the AF image sensor. The contrast detect autofocus (CDAF) is ae experimental algorithm that provides continuous focussing.

I am presenting this code because I couldn't find anything quite like this when I went looking. Gstreamer in glib applications written in C/C++ are promininet in NVIDIA's example code, with nvgstcapture-1.0 being one example. The Deepstream SDK has many C/C++ examples using glib applications. It seems obvious that a device with a prefectly functional GPIO should incorparate software examples to use it.

# Building the code
The code here is provided with JSON files so that it can be built and debugged in Visual Studio Code. As the Jetson Nano will forever use Ubuntu 18.04, I believe the latest version of Visual Studio Code to use is 1.85.2. I have used that on my Jetson Nano and it worked fine.

The dependencies you will need are:
* sudo apt install libgtk-3-dev
* sudo apt install i2c-tools
* sudo apt install libi2c-dev

Select the build all task and the application will build (less than a minute on the Nano).

## To run the application 'AS IS' you will need an autofocus IMX219 and an AS7265x demonstration board attached.

To trigger the fully timed sequence you will also need to trigger pin 7 on the GPIO. This can be done with a momentary switch and some resistors if you are using only short leads. For a longer lead, a schmidt trigger circuit was used. This is detailed in the HardwareX article (for now).

# Further Work
It is is hoped that more boards can be added and verified as functioning directly from the GPIO using this approach.

