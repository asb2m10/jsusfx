jsusfx -  Opensource Jesusonic FX implementation
================================================
jsusfx is an opensource implementation of the [JSFX](http://www.reaper.fm/sdk/js/js.php) 
scripting language that is available with [Reaper](http://www.reaper.fm).

*There is too much bugs to be usable, but should be good enough for regular use soon!*

While the original JSFX scripting language can do alot of thing in Reaper, 
this implementation is focusing on providing dsp scripting processing 
for other hosts (like pure-data and Max/MSP) and platforms.

This project comes with a subset of the original eel2 code from Cockos 
[WDL](http://www.cockos.com/wdl).

See [this](http://forum.cockos.com/showthread.php?t=27764) for great JSFX
examples.

Limitations
-----------
* Only supports 2 in / 2 out
* No midi support
* No GFX (the GFX section is just ignored)

Pure-data and Max implementation
--------------------------------
The external object is called jsusfx~. See the pd and max directory to 
see how to build them.

* It will check on the max/pd path to find the script file specified in the object creation arguments.
* Sliders are normalized to 0..1 for all 'parameters'. If you wish to use the original slider range values, put 0 as a second argument to the creation of the jsus~ object. If you want to normalized to 0..127, put 127 for the second argument.
* Turning on the DSP runs the @init section.
* To change a slider, you need to send the following message [slider <num> <value>]
* You can send the 'compile' message to reload your script. (Max only for now)
* If you double click on the object, you can edit the JSFX script (Max only)

Credits
-------
* The core of the language is from WDL (the authors of JSFX and Reaper)
