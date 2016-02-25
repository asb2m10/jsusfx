### Liteon's plugin pack for the JesuSonic platform.

# DISCUSSION

http://forum.cockos.com/showthread.php?t=27764

# PLUGIN LIST

* SonicEnhancer (sonic_enhancer)
  Modeled after a popular unit for sonic enhancement. Produces an all pass
  filter near 700hz, progressively delays all lower frequencies and also has
  a DC filter.
  - input/output gain
  - low contour - adds a low end shelf
  - process - adds a high end shelf
  - cv - program dependent control voltage for the process amount
  - noise floor - adds noise floor at around -96dBFS

* TubeHarmonics (tubeharmonics/tubeharmonics_amp)
  A rough model of a dual tube circuit stage. Not an exact tube model, but
  perhaps closer to the triode family. Adds controllable and transient aware
  amount of odd and even harmonics to the signal. The effect can be very subtle
  or quite drastic. For testing purposes use a periodic signal. The "amp"
  version of the fx is a tube pre-amp stage.
  - separate odd / even harmonics amount
  - fluctuation - circuit instability also controls the transient awareness 
  - TS input - tube stage input in decibels (level of input signal)
  - TS ouput - tube stage ouput in decibels (level of harmonics) 
  - output - master out 
  
* Lo-Fi (lo-fi)
  A mono/stereo bitcrusher, sample rate reducer with post filter.
  - bitcrusher/sample rate reducer  
  - 2 point linear interpolation
  - 12db/oct low-pass filter
  - pre amp
  - outgain  

* PhaseMeter (phasemeter)
  A goniometer with precision amount, scaling and color schemes.
  - 3 (funky) color schemes
  - precision (cpu usage related)
  - positive scale amount
  
* StereoTilt (stereotilt)
  Mono-to-stereo filter. Spectrum on left channel is tilted in one direction,
  while the spectum on the right channel is flipped in the other.  
  - center frequency
  - tilt amount
  - balance
  - outgain
  
* FARModulator (farmodulator)
  FM, AM/RM effect, which can use 2 sine waves as carrier/modulator, but also
  any other input signal as modulator. Can produce a wide palette of sounds.  
  - operation modes - FM, M1(mixed), M2(mixed)
  - modulator signal selection 
  - separate modulator/carrier gain   
  - output gain
  Note: the effect is mono
  
* PRNG-Plot (prngplot)
  A plotter algorithm for generated pseudo-random numbers.
  
* Non-Linear Processor (nonlinear)
  Simple non-linear processor. Roughly mimics analog circuit behaviour.
  - saturation amount - sinusoidal waveshaper. adds "odd" harmonics.
  - fluctuation amount - adds probability to all parameters in the model.
  - floor reduction - adds filtered white noise at a defined range, calculated  
    from bit depth.
  - output gain -24/+24db  
  The effect also models a basic frequency response from an analog prototype.
  Two slopes at the bottom and low end are present and also a positive
  low-shelf, around 300hz. But the probability also affect the filter parameters.
  The result from this could be characterized as "dynamic", as opposed to "static"
  where a transfer function is time invariant.
  The amount of probability is material dependant and slightly increases
  for transients, found in the signal.
  
* Tilt EQ (tilteq)
  Simple "tilt" equalizer circuit. It has only two controls: Boost/Cut and a
  center frequency. While boosting or cutting volume with a shelving filter
  above the center frequency, it does the opposite with the range below the
  center frequency. This can produce very quick changes on the spectrum.
  For extreme settings: -6db will result a lowpass, +6db will result a highpass.   

* NP1136 (np1136peaklimiter)
  Program dependant peak limiter. Full documentation here:  
  http://sites.google.com/site/neolit123/Home/np1136_manual.pdf

* De-Esser (deesser)
  De-Esser which uses "splt-band" compression. The crossover is constructed with
  second order Linkwitz-Riley filters.
  - stereo/mono processing
  - target type - compress a band or the whole top end.
  - monitor on/off - listen to the compressed signal
  - frequency range - 20 - 20khz
  - bandwidth in ocvaves - 0.1 - 3.1oct
  - threshold - 0 to -80db
  - ratio - 1:1 - 20:1 
  - 3 time constants.
  - output -inf/24dB - output gain

* Pseudo-Stereo (pseudostereo)
  Pseudo-Stereo fx based on 'mdaStereo' by Paul Kellet. Can be used for
  mono-to-stereo conversations. Uses one feedback delay on R ('Haas fx' mode)
  or 2 separate feedback delays for L & R ('Comb' mode). Very light on the CPU.
  - fx amount (%) - haas / comb 
  - delay (ms) - feedback delay in milliseconds 
  - balance - L / R channel amount
  - output -20/20dB - output volume
  
* RingModulator (ringmodulator)
  A simple ring modulator circuit emulation. Uses a sinewave as the modulation
  signal, which can be 'waveshaped' with a diode, so that only the positive 
  semi-periods of time sinewave pass trought. Has feedback and non-linearities.
  - stereo/mono processing
  - mod-signal diode - on/off
  - mod-signal frequency - 20-20khz
  - feedback - 0-100% - lets the signal be processed multiple times by the RM
  - non-linearities - 0-100% - adds small variations to the mod-frequency and
  feedback amount.
  - mix - 0-100% - mixes the orginal signal with the processed signal.
  - output -inf/40dB - output volume
  - oversampling (on/off)

* StateVarible (morphing) filter (statevariable)
  Filter which uses x,y pads to morph between different states - LP, HP, BP, BR.
  - stereo/mono processing
  - lp,hp,bp,br modes
  - frequency range - 20-20000hz
 - res - resonance amount
 - filter amount - mixes the original signal with the filtered one.

* AppleFilter v.2 (applefilter72db)
  Original filter from apple.com AU tutorial. Modification allows up to
  12pole cascade (HP, LP).  
 - stereo/mono processing
 - slope - off/12db-72db per octave
 - frequency range - 20-20000hz
 - res - resonance amount -16/+16db 
 - output gain control -24/+24db

* 3BandPeakFilter (3bandpeakfilter)
  Filter bank containing two biquad peak filters from Stanley A. White's
  algorithms (JAES versions). Each filter provides three fully parametric bands.
  The plugin can be used as a three band EQ. Saturation control is also
  available.
  - two filter types: PF-3A, PF-3B. The two filters have similar behavior in   
  the midrange, but quite different at the low and high end.
  - stereo/mono processing
 - frequency ranges - 20-20000hz
 - gain -18/+18db
 - 12db lowpass, highpass
 - output gain control -24/+24db
 - saturation amount - 0-100% - adds harmonics and noise floor to the signal.
 - oversampling (on/off)

* VUMeterGFX / VUMeterGFXSum (vumetergfx/vumetergfxsum)
  Vintage-style VU meter with response and release controls. Uses the GFX
  section to draw all graphics in realtime. Summed (L+R) and stereo versions.
  - response control (MS) - RMS and peak meter response.
  - release - needle fallback time. 

* LorenzAttractor (lorenzattractor)
  Synthesizer based on Lorenz Attractor formulas. Has two oscillators: one sine
  wave, one square wave. There are various parameters that control both the
  sound and the plotted graphics. Can be used to produce ambient sounds.
  - rate - controls the rate at which the plotter/modulation is working.
  - plot osc 1+2/1 - when 'lines' is selected plotter outputs lines and the
  sound is a mix between osc 1 and 2 (sine+square). Otherwise when 'dots'
  is selected the output is a sine-wave only.
  - prandtl number - first parameter controling the attractor.
  - reileght number - second parameter.
  - color - changes the pallette of the plotter. When set to the farleft,
  modulation amount of osc1 is minimum. Otherwise its set to maximum.
  - tune - tunage of osc1 and osc2. 
  - output gain control -inf/+25dB 
  
* ShelvingFilter (shelvingfilter)
 Plugin with LowShelf and HighShelf biquad filters based on James A. Moorer's
  formulas.
 - stereo/mono processing 
 - frequency ranges - 20-20000hz
 - gain -12/+12db
 - output gain control -24/+24db
 
* PresenceEQ (presenceeq)
 Topend EQ based on James A. Moorer's formulas. Can add presence to the top end
  of sounds. Bandwidth of the boost is somehow smart and frequency dependant.
  Good sound.
 - stereo/mono processing 
 - frequency range - 3100hz-1850hz
 - cut/boost - -15/+15db
 - BW - bandwidth of the boost
 - output gain control -24/+24db
 
* BassManager (filename: bassmanager)
 This is a plugin for managing your bass samples. The idea behind this plugin
  is to make bass samples more present in the mix. It has full control over the
  low end. Sounds can be processed in stereo or mono. It has a 2pole lowshelf
  filter for boosting frequencies. Also a build in saturator, a control for
  high-end muffle and a limiter.
 - stereo/mono processing
 - spread - controls the width of the lowshelf
 - frequency - 30-250hz
 - boost - amount of boost for the low end - 0/24db
 - drive - adds saturation
 - muffle - "muffles" sharp highend frequencies.
 - output - controls the output gain -24/+24db
 - hipass - hipass filter for the low end at given frequency.
 - limiter(on/off) - to limit the output from the plugin
 - oversampling (on/off)

* Butterworth Filter (filename: butterworth24db)
 Classic sounding 24db filter model!
 - stereo/mono processing
 - filter mode - LP/HP
  - cutoff frequency - 20-20000hz
  - resonance amount - 0-0.9
  - limiter(on/off) - to limit the output from the plugin
  - output gain control -24/+24db

  ! This plugin comes with a warning !
 Due to the internal filter architecture this pluging may behave as unstable
 when cutoff frequency is automated very fast in the low end. A limiter will
 automatically kick-in in such cases so that user speakers aren't blown out. :)

* Chebyshev Filter - Type1 (filename: cheby24db)
  Classic sounding 24db filter model! With a very specific resonance.
  - stereo/mono processing
  - filter mode - LP/HP/BP
  - cutoff frequency - 20-20000hz
  - passband ripple amount - 0-0.9 (Produces two noticeable peaks on the
  spectrum)
  - limiter(on/off) - to limit the output from the plugin
  - output gain control -24/+24db

* Moog Filter (filename: moog24db)
  Classic sounding 24db filter plugin modeled after the infamous Moog Filter.
  - stereo/mono processing
  - filter mode - LP/HP/BP
  - cutoff frequency - 20-20000hz
  - resonance amount - 0-0.85
  - drive - adds saturation to the signal
  - limiter(on/off) - to limit the output from the plugin
  - output gain control -24/+24db
  - oversampling (on/off)
 
* Pink Noise Generator (filename: pinknoisegen)
 A simple pink noise generator with mono/stereo output.
  - stereo/mono - one/two channels noise output
  - output gain control -24/+24db

* RBJ Stereo Filter (filename: rbjstereofilter12db)
  A filter that controls only the stereo image of a sound. Useful for precise
  sound modeling. Based on RBJ filters cookbook. Includes saturation.
  - filter amount - controls mix between original signal and filtered one. 
  - HP - 12db hipass for the stereo image at given frequency. 
  - LP - 12db lowpass for the stereo image at given frequency. 
  - drive - saturation for the stereo image.
  - side - stereo image width.
  - mid - mid amount.
  - output gain control (m+s) -24/+24db
  - oversampling (on/off)

* Simple 6db LP Filter (filename: simplelp6db)
  A simple 6db LP filter. Good for less steeper cuts of high end sounds.
  CPU friendly and good for automation. No resonance control.
  - mono/stereo processing
  - cutoff frequency - 20-20000hz
  - output gain control -24/+24db
 
* WaveshaperMulti (filename: waveshapermulti)
  A waveshaper bank with different waveshaper formulas.
  - mono/stereo processing
  - type - selected waveshaper formula
  - drive - controls amount of saturation
  - muffle - "muffles" sharp highend frequencies.
  - output gain control -24/+24db
  - limiter(on/off) - to limit the output from the plugin
  - oversampling (on/off) 

# TERMS OF USE:

NO WARRANTY IS GRANTED. THESE PLUG-INS ARE PROVIDED WITHOUT WARRANTY OF ANY
KIND. NO LIABILITY IS GRANTED, INCLUDING, BUT NOT LIMITED TO, ANY DIRECT OR
INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGE ARISING OUT OF THE
USE OR INABILITY TO USE THESE PLUG-IN, COMPUTER FAILTURE OF MALFUNCTION
INCLUDED. THE USE OF ANY SOURCE CODE, EITHER PARTIALLY OR IN TOTAL, IS GRANTED.
FURTHERMORE ARE THESE PLUG-INS A THIRD PARTY CONTRIBUTION EVEN IF INCLUDED
IN REAPER(TM), COCKOS INCORPORATED OR ITS AFFILIATES HAVE NOTHING TO DO WITH
THEM. LAST BUT NOT LEAST, BY USING THE PLUG-INS YOU RELINQUISH YOUR CLAIM TO SUE
IT'S AUTHOR, AS WELL AS THE CLAIM TO ENTRUST SOMEBODY ELSE WITH DOING SO.

# LICENSE

THE PLUGINS ARE RELEASED UNDER GPL.
SEE THE GNU GENERAL PUBLIC LICENSE FOR MORE DETAILS.
YOU SHOULD HAVE RECEIVED A COPY OF THE GNU GENERAL PUBLIC LICENSE
ALONG WITH THIS PACKAGE. IF NOT, VISIT: http://www.gnu.org/licenses/

# REFERENCES

EACH PLUGIN MAY INCLUDE SOURCE CODE OR FORMULAS WHICH ARE PROVIDED FOR PUBLIC
USE BY THEIR ORIGINAL AUTHORS. SOME PLUGINS MAY REFER TO ORIGINAL SOURCE CODE
OR PAPERS IF THERE ARE ANY. PLUGINS IN THE PACK MAY INCLUDE CODE OR FORMULAS
WHICH ORIGINATED FROM THIS AUTHOR. USING SUCH IN THIRD PARTY OPEN SOURCE OR ANY
COMMERCIAL PROJECTS IS FULLY GRANTED UNDER THE TERMS OF THE GPL LICENSE.
REDISTRIBUTION IN ANY FORM, WITHOUT THE PERMISSION OF THE AUTHOR IS PROHIBITED.

# AUTHOR

Lubomir I. Ivanov (liteon)
neolit123@gmail.com
http://neolit123.blogspot.com

23.02.12
- added sonic_enhancer
31.03.11
- some fixes
20.07.10
- fix in vumetergfxsum to show better rms values
28.05.10
- small tweaks
- clarification in moog, cheby, butter that some of the filter
  modes are 6db/oct
28.11.09
- added tubeharmonics_amp
24.11.09
- added tubeharmonics
- fix for pdc in nonlinear
- added input gain for nonlinear
- small tweak in vumetergfxsum
06.10.09
- added stereotilt
- added lo-fi
- added phasemetergfx
04.09.09
- added farmodulator
- added prngplot
06.07.09
- shelvingfilter - changed to -24/+24db range.
03.07.09
- added nonlinear processor
- various minor tweaks here and there
10.06.09
- np1136 - improved 'british mode'.
- deesser - small tweak for the time constants.
29.05.09
- added tilteq
- added np1136
- added deesser
- minor fix for pseudo stereo gain.
15.04.09
- OSx2 bandlimit fir filter
- various small fixes
14.04.09
- added pseudostereo 
- small fix for 3bandpeakfilter's LP.
- OSx2 on/off switch for plugins
05.04.09
- VUMeterGFX - added inertial fallback, gfx height fix for Reaper3. 
- SimpleLP6db - added HP,LP switch
- WaveshaperMulti - added 2x oversampling
- Moog - added 2x oversamling, parameter interpolation
- Cheby - added parameter interpolation
- BassManager - added 2x oversamling
- 3BandPeakFilter - added 2x oversamling
- RingModulator - added 2x oversamling (with on/off switch)
- RBJStereoFilter - added 2x oversamling, separate mid/side amounts 
15.03.09
- added ring modulator 
06.03.09
- VUMeterGFXsum small fix
02.03.09
- small fix for StateVarible - update paramenters
02.03.09
- added StateVarible
13.02.09
- replaced applefilter with v.2 - up to 12pole hp, lp
- 3bandpeakfilter: reduced noise floor from the 'saturation' control
- 3bandpeakfilter: added 12db hp, lp - same as apple filter
- vu meter: default response time - 50ms
- pinknoisegen: should work with all samplerates now 
23.01.09
- added 3BandPeakFilter
- added VUMeterGFXSum (L+R) version. Also RMS window and Release/Response
controls.
- small fix for BassManager's HP filter. (Can cut up to 300hz now)
29.12.08
- added VUMeterGFX
20.12.08
- added -15db cut to the PresenceEQ
08.12.08
- added LorenzAttractor
14.11.08
- added ShelvingFilter
09.11.08
- small fix for the applefilter - output gain
06.11.08
- added exponential control to all full range frequency sliders (20-20k)
"F = xxxxx" text in the GFX section to display the actual frequency.
- added smoothing (ctrl+mouse drag)
- added output gain control (-25/25db) to all filters
27.10.08 
- added PresenceEQ & AppleFilter
24.10.08
- first packed release.

