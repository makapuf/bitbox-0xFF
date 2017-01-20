#!/usr/bin/python2
import Image, ImageDraw

img=Image.new('P',(256,256),255)
img.palette=Image.open('pal_micro.png').palette
imgdraw = ImageDraw.Draw(img)

for i in range(16) : 
	for j in range(16) : 
		imgdraw.rectangle((16*i,16*j,16*i+15,16*j+15),outline=j*16+i)
		imgdraw.rectangle((16*i+1,16*j+1,16*i+14,16*j+2),outline=j*16+i)
		imgdraw.text((16*i+3,16*j+2),"%02x"%(j*16+i))
img.save('pattern.bmp')
