jsfx~ - Jesusonic FX for Pure Data
======================================
The jsusfx implementation is done through 2 externals called `jsfx~` and `jsusfx~`.

* `jsfx~` is the runtime object to expose JSFX scripts in pure-data. It expect as object 
creation argument the script to use that should be in the pd path. Upon object creation, 
each of the sliders will be exposed as an inlet.
** Use [describe] to output the associated sliders
** Use [dumpvars] to dump the current variables values
** Use [bypass 0/1] to bypass the effect
* `jsusfx~` is used for script development and can switch script with the command compile. All
the slider parameter are read trought the slider message. At object creation, you can also put 
the number of inlet/outlet the script is expected to use. If you don't specify it, it will 
count the number of time in_pin and out_pin is used.
** To change a slider, you need to send [slider <slider id> <0..1 value>]
** Sliders are normalized to 0..1 for all parameters (for the jsusfx~ object)
** Use [compile] message to recompile your script. Optionally you can specify a new script to compile
** Use [describe] to output the associated sliders
** Use [dumpvars] to dump the current variables values
** Use [bypass 0/1] to bypass the effect

Version 0.4
-----------
* Multi-channel support
* Native ARM support (Raspberry Pi) 
* File API support
* @import support
* Subpatch wrapper generator (see jsfx2patch.py) 
* More support of extended sliders
* Various bug fixes
* CMake now global build system

Subpatch generator
------------------
Since jsusfx_pd doesn't support pd_loader, a Python script can be use to generate a patch based
on the content of a JSFX script. This patch can then be used as an abstraction wrapper to ease 
the integration.

To run the script, run the command below. Once it is done, it should generate a .pd file with same
name as the script. 

```
$ python jsfx2abstract.py <jsfx script>
```

Limitations
-----------
* @gfx, @serialize section is ignored
* No midi support

BUILDING
--------
* cmake, PHP and nasm is required to build native x86_64 support code

```
$ git clone --recurse-submodules https://github.com/asb2m10/jsusfx.git
$ cd jsusfx/pd
$ cmake .
$ make install
```
