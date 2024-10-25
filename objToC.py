#!/bin/env python3

import sys
import os

def main():
	if(len(sys.argv) != 2):
		return 1

	filePath = sys.argv[1]

	if not os.path.exists(filePath):
		return 1

	f = open(filePath, mode='r')
	lines = f.readlines()
	f.close()

	vertices = []
	indices = []

	for l in lines:
		if(l[0] == '#'):
			continue

		lineData = l.split(' ')
		if(lineData[0] == "v"):
			for i in range(1,4):
				vertices.append(float(lineData[i]))
		if(lineData[0] == "f"):
			for i in range(1,4):
				faceElements = lineData[i].split('/')
				indices.append(int(faceElements[0])-1)


	print("#include <gccore.h>")
	print(f"u32 vertCount = {len(vertices)};");
	print(f"u32 indexCount = {len(indices)};");
	print("f32 vertPositions[] ATTRIBUTE_ALIGN(32) = {")
	for v in vertices:
		print(f"{v},", end='')
	print("\n};")
	print("u16 vertIndices[] ATTRIBUTE_ALIGN(32) = {")
	for i in indices:
		print(f"{i},", end='')
	print("\n};")

	return 0

if __name__ == "__main__":
	exit(main())

