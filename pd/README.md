jsusfx_pd - JesuSonic FX for Pure Data
======================================
The external object is called jsusfx~ and the specify the 
script to run. This script is search trough your pd/max path.

You can also put the number of inlet/outlet the script is expected
to use. If you don't specify it, it will count the number of time
in_pin and out_pin is used.

* To change a slider, you need to send [slider <num> <0..1 value>]
* Sliders are normalized to 0..1 for all parameters
* Turning on the DSP runs the @init section
* Use [compile] message to recompile your script. Optionally you can specify a new script to compile
* Use [describe] to output the associated sliders
* Use [dumpvars] to dump the current variables values
* Use [bypass 0/1] to bypass the effect

Version 0.4
-----------
* Multi-channel support
* Native ARM support (Raspberry Pi) 
* File API support
* @import support
* More support of extended sliders
* Various bug fixes
* CMake now global build system

Limitations
-----------
* @gfx, @serialize section is ignored
* No midi support

BUILDING
--------
* cmake, PHP and nasm is required to build native x86_64 support code

`$ cd pd
 $ cmake .
 $ make
`
