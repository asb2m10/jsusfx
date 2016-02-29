jsusfx -  Opensource JesuSonic FX implementation
================================================
jsusfx is an opensource implementation of the [JSFX](http://www.reaper.fm/sdk/js/js.php) 
scripting language that was created by [Cockos](http://www.cockos.com/jesusonic/) and
is made available with [Reaper](http://www.reaper.fm).

While the original JSFX scripting language can do alot of things in Reaper, this 
implementation is focusing on providing dsp scripting processing for other hosts
(like pure-data and Max/MSP) and platforms.

This project comes with a subset of the original eel2 code from Cockos 
[WDL](http://www.cockos.com/wdl).

While this project could support plugin formats like LV2 or VST, this 
implementation focuses on Pure Data and Max support.

Version 0.3
-----------
* Native x86 x86_64 for OS X and Linux (10 times faster than portable)
* gcc generated code now works at runtime

Version 0.2
-----------
* Add OS X as a target for Pure Data external

Pure Data and Max implementation
--------------------------------
The external object is called jsusfx~ and the object arguments are the 
script to run. This script is search trough your pd/max path.

* To change a slider, you need to send [slider <num> <0..1 value>]
* Sliders are normalized to 0..1 for all parameters
* Turning on the DSP runs the @init section
* Use [compile] message to recompile your script. Optionally you can specify a new script to compile
* Use [describe] to output the associated sliders
* Use [dumpvars] to dump the current variables values
* Use [bypass 0/1] to bypass the effect
* Double click on the object and you can edit the JSFX script (Max only)

See the pd and max directory to see how to build them.

Limitations
-----------
* Only supports 2 in / 2 out
* @gfx, @serialize and @import section is ignored
* No midi support

BUILDING
--------
* PHP and nasm is required to build native x86_64 support code

Credits
-------
* The core of the language is from WDL (the authors of JSFX and Reaper)
