jsusfx -  Opensource Jesusonic FX implementation
================================================
jsusfx is an opensource implementation of the [JSFX](http://www.reaper.fm/sdk/js/js.php) 
scripting language that was created by [Cockos](http://www.cockos.com/jesusonic/) and
is made available with [Reaper](http://www.reaper.fm).

While the original JSFX scripting language can do a lot of things in Reaper, this 
implementation is focusing on providing dsp scripting processing for other hosts
(like pure-data and Max/MSP) and platforms.

This project comes with a subset of the original eel2 code from Cockos 
[WDL](http://www.cockos.com/wdl).

While this project could support plugin formats like LV2 or VST, this 
implementation focuses on Pure Data support. Support for version 0.4 is
in progress for Pure Data. See subdirectory [pd](pd).

Marcel Smit, who is also working on [Framework](https://github.com/marcel303/framework) 
that uses JsusFx, greatly contributed on version 0.4, see below video. 

[![Framework](https://img.youtube.com/vi/7f9fOeBecaY/0.jpg)](https://www.youtube.com/watch?v=7f9fOeBecaY)

Version 0.4
-----------
* Multi-channel support
* File API support
* @import and @gfx section support
* Midi support
* More support of extended sliders
* Various bug fixes
* Native ARM support
* CMake now global build system

Version 0.3
-----------
* Native x86 x86_64 for OS X and Linux (10 times faster than portable)
* gcc generated code now works at runtime

Limitations
-----------
* @serialize section is ignored

Building
--------
* cmake is the build system and PHP and nasm are required to build native x86_64 support code

Credits
-------
* @marcel303 (Marcel Smit) did a lot of work (Version 0.4) in implementing the missing features from the previous versions
* The core of the language is from WDL (the authors of JSFX and Reaper)
