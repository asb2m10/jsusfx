"""
 * Copyright 2014-2018 Pascal Gauthier
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
import datetime

class Patch :
	def __init__(self, scriptName) :
		self.items = []
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

	def connect(self, source, outlet, dest, inlet) :
		srcId = self.items.index(source);
		destId = self.items.index(dest);
		self.fd.write("#X connect %d %d %d %d;\r\n" % (srcId, outlet, destId, inlet));

	def close(self) :
		self.fd.close();

class PdObj :
	def __init__(self, args) :
		self.args = args;
		self.objnum = -1;

def makeSlider(name, mn, mx, default) :
	name = name.replace(" ", "_");
	return PdObj("hsl 128 15 %d %d 0 1 empty empty %s -2 -6 0 8 -262144 -1 -1 %d 1" % (mn, mx, name, default));

if __name__ == "__main__" :
	if len(sys.argv) < 2 :
		print("Missing jsfx file");
		sys.exit(1);

	script = os.path.basename(sys.argv[1]);
	patch = Patch(script);
	sliders = [];
	desc = "";
	pinIns = 0;
	pinOuts = 0;

	for i in open(sys.argv[1], "r").readlines() :
		if i[0] == '@':
			break

		if i.startswith("slider") :
			slider = i.split(":")
			id = slider[0][5:]
			default = float(slider[1].split("<")[0])

			if len(slider[1].split(">")) > 1 :
				name = slider[1].split(">")[1].rstrip("\r\n");
			else :
				name = "undefined"

			sliderRange = slider[1].split("<")[1].split(">")[0].split(",")

			mn = float(sliderRange[0])
			mx = float(sliderRange[1])

			steps = 127.0 / (-mn + mx)
			default = (default + -mn) * steps * 100

			sliders.append(makeSlider(name, mn, mx, int(default)))

		elif i.startswith("desc") :
			patch.addText(10, 5, "JSFX:%s" % i[5:].rstrip("\r\n"))
		elif i.startswith("in_pin") :
			if pinIns != -1 :
				pinIns += 1
			elif i.startswith("in_pin:none") :
				pinIns = -1
		elif i.startswith("out_pin") :
			if pinOuts != -1 :
				pinOuts += 1
			elif i.startswith("out_pin:none") :
				pinOuts = -1

	patch.addText(10, 20, "================================================")
	patch.addText(10, 35, "Generated from jsfx2path.py on %s" % datetime.datetime.now())

	sliderX = -137
	sliderY = 80

	if pinIns == 0 :
		pinIns = 2;
	elif pinIns == -1 : 
		pinIns = 1;
	if pinOuts == 0 :
		pinOuts = 2;
	elif pinOuts == -1 :
		pinOuts = 0;

	for i in sliders :
		if sliderX > 450 :
			sliderY += 50
			sliderX = 13
		else :
			sliderX += 150

		patch.add(sliderX, sliderY, i)

	fxobj = PdObj("jxrt~ %s" % script)
	patch.add(30, sliderY + 60, fxobj)

	inlets = []
	for i in range(pinIns) :
		dspInlet = PdObj("inlet~")
		inlets.append(dspInlet)
		patch.add(30, sliderY + 40, dspInlet)

	outlets = []
	for i in range(pinOuts) :
		dspOutlet = PdObj("outlet~")
		outlets.append(dspOutlet)
		patch.add(30, sliderY + 83, dspOutlet)

	for i in sliders :
		patch.connect(i, 0, fxobj, pinOuts + sliders.index(i))

	for i in inlets :
		patch.connect(i, 0, fxobj, inlets.index(i))

	for i in outlets :
		patch.connect(fxobj, outlets.index(i), i, 0)

	patch.close();

sys.exit(0);
