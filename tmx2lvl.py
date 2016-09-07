#!/usr/bin/python2
import sys
import os.path
import argparse
import array
import xml.etree.ElementTree as ET

from PIL import Image 

from lvl2tmx import parse_defs

sys.argv.append('out.tmx')

parser = argparse.ArgumentParser(description='Process TMX files to bmp level file')
parser.add_argument('file',help='input .tmx filename')
args = parser.parse_args()

PALETTE = os.path.dirname(__file__)+'/pal_micro.png'
DEFS = parse_defs(True)
TRANSP=230

tree = ET.parse(args.file)
root = tree.getroot()

minimap = [[TRANSP]*16 for i in range(16)]
properties=[[TRANSP]*8 for i in range(32)]

# tileset
# -----------------------------------------------------

# open tileset as the basis of the output file.
# tileset must be 256x256. transparent is color 230, #4800da
tileset = root.find('tileset') # only one tileset, 16x16 
assert tileset.get("tilewidth")==tileset.get("tileheight")=='16', "tiles must be 16x16"
assert tileset.get("tilecount")=='256',"tileset must be 256x256 (16 by 16 tiles), it has %s tiles"%tileset.get("tilecount")

tileset_imagename = os.path.join(os.path.dirname(os.path.abspath(args.file)),tileset.find("image").get("source"))
tileset_image=Image.open(tileset_imagename)

assert tileset_image.size == (256,256)
assert tileset_image.mode == 'P','image must be 256 colors'

# write tile terrains to minimap
terrain_names = [ter.get('name') for ter in tileset.findall('terraintypes/terrain') ]
terrain_ids   = [DEFS['terrains'][tn] for tn in terrain_names]

# tile data
for t in tileset.findall('tile') : 
	tid = int(t.get('id'))
	terrains = set(t.get('terrain').split(','))
	assert len(terrains)==1,'several terrains defined for tile %d (%d,%d)'%(tid,tid%16,tid//16)
	minimap[tid//16][tid%16]=terrain_ids [int(terrains.pop())]

# make a copy for resulting image
dest_img = tileset_image.copy()
dest_pixels = dest_img.load()


# reserve tileset data in minimap if there is one non-empty pixel originally in tilemap


# export tilemap to image
# -----------------------------------------------------
lvlid=0
for tilemap in root.findall("layer") : # intro,1,2,3,4
	lvlname=tilemap.get('name')
	if lvlname.startswith('_') : continue
	print lvlname,lvlid,

	assert tilemap.get("width")  == tilemap.get("height") == '256','tilemap must be 256x256'
	levelc = [255,249,7,226,121][lvlid]
	properties[lvlid][0]=levelc

	data = tilemap.find("data")
	if data.get('encoding')=='csv' :
	    indices = [int(s) for s in data.text.replace("\n",'').split(',')]
	elif data.get('encoding')=='base64' and data.get('compression')=='zlib' :
	    indices = array.array('I',data.text.decode('base64').decode('zlib'))
	else :
	    raise ValueError,'Unsupported layer encoding :'+data.get('encoding')

	for n,tile_id in enumerate(indices) : 
		if tile_id > 0 : 
			x,y=n%256,n//256
			dest_pixels[x,y] = tile_id-1
			# reserve/check data in minimap 
			assert minimap[y//16][x//16] in (TRANSP,levelc),'conflict in minimap for level %s in (%d,%d) : had %d' %(lvlname,x//16,y//16,minimap[y//16][x//16])

			minimap[y//16][x//16]=levelc
	lvlid += 1

# export minimap
# -----------------------------------------------------
for ym,l in enumerate(minimap) : 
	for xm,c in enumerate(l) : 
		dest_pixels[xm,240+ym] = c

# finally save it as bmp 256c, non compressed
outfilename = args.file.rsplit('.',1)[0]+'.bmp'
dest_img.save(outfilename)
print "saved to %s"%outfilename

