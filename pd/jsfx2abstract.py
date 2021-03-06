"""
 * Copyright 2018 Pascal Gauthier
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
"""

import os;
import os.path;
import sys;
import datetime;

class Patch :
	def __init__(self, scriptName) :
		self.items = []
		print("Processing %s" % os.path.abspath(scriptName))
		filename = scriptName.split(".")[0]
		self.fd = open("%s.pd" % filename, "w")
		self.fd.write("#N canvas 50 50 605 405 10;\r\n")

	def add(self, x, y, obj) :
		self.items.append(obj)
		self.fd.write("#X obj %d %d %s;\r\n" % (x, y, obj.args))
		obj.objnum = len(self.items)

	def addText(self, x, y, msg) :
		self.items.append(None)
		self.fd.write("#X text %d %d %s;\r\n" % (x, y, msg))

	def addMsg(self, x, y, msg) :
		self.items.append(msg)
		self.fd.write("#X msg %d %d %s;\r\n" % (x, y, msg.args))

	def connect(self, source, outlet, dest, inlet) :
		srcId = self.items.index(source);
		destId = self.items.index(dest);
		self.fd.write("#X connect %d %d %d %d;\r\n" % (srcId, outlet, destId, inlet));

	def close(self) :
		self.fd.close();

class PdObj :
	def __init__(self, args) :
		self.args = args;

class PdMsg :
	def __init__(self, args) :
		self.args = args;


def makeSlider(name, mn, mx, default, id) :
	name = name.replace(" ", "_");
	if name == "" :
		name = "undefined";
	obj = PdObj("hsl 128 15 %d %d 0 1 empty empty %s -2 -6 0 8 -262144 -1 -1 %d 1" % (mn, mx, name, default));
	obj.id = id
	return obj


def makeRadio(name, step, default, id) :
	name = name.replace(" ", "_")
	if name == "" :
		name = "undefined"

	obj = PdObj("hradio 15 1 0 %d empty empty %s 0 -8 0 8 -262144 -1 -1 %d" % (step, name, default));
	obj.id = id
	return obj	

if __name__ == "__main__" :
	if len(sys.argv) < 2 :
		print("Missing jsfx file");
		sys.exit(1);

	script = os.path.basename(sys.argv[1]);
	patch = Patch(script);
	sliders = [];
	desc = "";
	pinIns = [];
	pinOuts = [];
	extraSliders = [];

	for i in open(sys.argv[1], "r").readlines() :
		if i[0] == '@':
			break

		if i.startswith("slider") :
			slider = i.split(":");
			id = int(slider[0][6:]);
	
			if len(slider[1].split(">")) > 1 :
				name = slider[1].split(">")[1].rstrip("\r\n");
			else :
				name = "undefined";

			sliderRange = slider[1].split("<")[1].split(">")[0].split(",");
			try:
				default = slider[1].split("<")[0]
				if '=' in default :
					default = float(default.split("=")[1]);
				else :
					default = float(default);
			except :
				default = float(sliderRange[0]);

			if len(sliderRange) > 2 :
				stepper = sliderRange[2];
				if '{' in stepper :
					stepper = stepper.split("{")[0];
				try :
					stepper = float(stepper);
				except :
					stepper = -1;
			else :
				stepper = -1;

			mn = float(sliderRange[0]);
			mx = float(sliderRange[1]);

			if stepper == 1 and mn == 0 and mx <= 8 :
				sliders.append(makeRadio(name, mx+1, int(default), id))
			else :
				steps = 127.0 / (-mn + mx);
				default = (default + -mn) * steps * 100;
				sliders.append(makeSlider(name, mn, mx, int(default), id));

		elif i.startswith("desc") :
			patch.addText(10, 5, "JSFX:%s" % i[5:].rstrip("\r\n"));
		elif i.startswith("in_pin") :
			if pinIns != None :
				pinIns.append(i[8:]);
			elif i.startswith("in_pin:none") :
				pinIns = None;
		elif i.startswith("out_pin") :
			if pinOuts != None :
				pinOuts.append(i[9:]);
			elif i.startswith("out_pin:none") :
				pinOuts = None;

	patch.addText(10, 20, "Generated from jsfx2abstract.py on %s" % datetime.date.today())
	patch.addText(10, 35, "================================================")

	sliderX = -137;
	sliderY = 80;

	if pinIns == None :
		pinIns = [];
	elif len(pinIns) == 0 :
		pinIns.append("input_l");
		pinIns.append("input_r");
	if pinOuts == None :
		pinOuts = [];
	elif len(pinOuts) == 0 :
		pinOuts.append("output_l");
		pinOuts.append("output_r");

	for idx, val in enumerate(sliders) :
		if sliderX > 450 :
			sliderY += 50;
			sliderX = 13;
		else :
			sliderX += 150;

		patch.add(sliderX, sliderY, val);
		# PD has a limitation of 17 inlet, we have to "front" the other ones...
		if idx + len(pinIns) > 17 :
			msg = PdMsg("uslider %d \\$1" % val.id);
			patch.addMsg(sliderX, sliderY + 17, msg);
			extraSliders.append(msg);

	fxobj = PdObj("jsfx~ %s" % script);
	patch.add(30, sliderY + 65, fxobj);

	inlets = [];
	for i in range(len(pinIns)) :
		dspInlet = PdObj("inlet~");
		inlets.append(dspInlet);
		patch.add(30, sliderY + 42, dspInlet);

	midiin = PdObj("inlet");
	patch.add(80, sliderY + 42, midiin);

	outlets = []
	for i in range(len(pinOuts)) :
		dspOutlet = PdObj("outlet~");
		outlets.append(dspOutlet);
		patch.add(30, sliderY + 88, dspOutlet);

	midiout = PdObj("outlet")
	patch.add(80, sliderY + 88, midiout)

	patch.connect(midiin, 0, fxobj, 0)
	patch.connect(fxobj, len(outlets), midiout, 0)

	extraIdx = 0;
	for idx, i in enumerate(sliders) :
		if idx+len(pinIns) > 17 :
			patch.connect(i, 0, extraSliders[extraIdx], 0);
			patch.connect(extraSliders[extraIdx], 0, fxobj, 0);
			extraIdx += 1;
		else :
			patch.connect(i, 0, fxobj, len(pinOuts) + idx);

	for i in inlets :
		patch.connect(i, 0, fxobj, inlets.index(i));

	for i in outlets :
		patch.connect(fxobj, outlets.index(i), i, 0);

	patch.close();
