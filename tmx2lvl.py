#!/usr/bin/python2
import sys
import os.path
import argparse
import array
import xml.etree.ElementTree as ET

from PIL import Image 
from lvl2tmx import parse_defs

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
player_tiles=[] # tiles that are marked as tile=player

oid=5 # first object position in properties map
for t in tileset.findall('tile') : 
	tid = int(t.get('id'))
	
	terrains = t.get('terrain')
	if terrains is not None : 
		terrains = set(terrains.split(','))
		assert len(terrains)==1,'different terrains defined for tile %d (%d,%d)'%(tid,tid%16,tid//16)
		minimap[tid//16][tid%16]=terrain_ids[int(terrains.pop())]


	# if the tile has some properties, it is an object 
	tile_properties = {p.get("name"):p.get("value") for p in t.findall('properties/property')}
	if 'collision' in tile_properties or 'movement' in tile_properties : 
		movement = tile_properties.get('movement','nomove')
		collision = tile_properties.get('collision','none')
		spawn = int(tile_properties.get('spawn',230))
		properties[oid] = [
			tid, 
			DEFS['movements'][movement], 
			DEFS['collisions'][collision], 
			spawn, # by name ??
			230,230,230,230, # alt collisions ..
			]
		print oid,movement,collision
		oid += 1 # next property

		if movement=='player' : 
			player_tiles.append(tid)

# make a copy for resulting image
dest_img = tileset_image.copy()
dest_pixels = dest_img.load()


# reserve tileset data in minimap if there is one non-empty pixel originally in tilemap


# export tilemap to image
# -----------------------------------------------------
lvlid=0
for tilemap in root.findall("layer") : # intro,1,2,3,4 - skipping bad ones TODO make a real map
	lvlname=tilemap.get('name')
	if lvlname.startswith('_') : 
		continue
	elif lvlname=="intro" : # or music
		lvlid=None
		levelc=255
	elif lvlname.startswith('level') : 
		lvlid = int(lvlname[-1])-1
		levelc = [249,7,226,121][lvlid]
		properties[lvlid][0]=levelc
	else : 
		RaiseValueError, "layers must be called 'intro', 'level 1' .. 'level 4','music' or start with an underscore(_)"
		
	assert tilemap.get("width")  == tilemap.get("height") == '256','tilemap must be 256x256'

	data = tilemap.find("data")
	if data.get('encoding')=='csv' :
	    indices = [int(s) for s in data.text.replace("\n",'').split(',')]
	elif data.get('encoding')=='base64' and data.get('compression')=='zlib' :
	    indices = array.array('I',data.text.decode('base64').decode('zlib'))
	else :
	    raise ValueError,'Unsupported layer encoding :'+data.get('encoding')

	if lvlid is not None :
		layer_properties = {p.get("name"):p.get("value") for p in tilemap.findall('properties/property')}
		
		# level properties
		properties[lvlid][2]=DEFS['control types'][layer_properties.get("control","classic")]
		properties[lvlid][3] = int(layer_properties.get('accel_x',4))+int(layer_properties.get('accel_y',8))*16
		properties[lvlid][4] = int(layer_properties.get('maxspeed_x',8))+int(layer_properties.get('maxspeed_y',8))*16
		properties[lvlid][5] = int(layer_properties.get('alt_accel_x',0))+int(layer_properties.get('alt_accel_y',4))*16
		properties[lvlid][6] = int(layer_properties.get('alt_maxspeed_x',0))+int(layer_properties.get('alt_maxspeed_y',4))*16

		# music instr defs as properties of level
		for instr in range(1,5) : 
			waveform=DEFS['instruments properties'][layer_properties.get('waveform%d'%instr,'square25')]
			volume = int(layer_properties.get('volume%d'%instr,'8'))
			decay  = int(layer_properties.get('decay%d'%instr,'0'))
			sustain= int(layer_properties.get('sustain%d'%instr,'4'))

			dest_pixels[3*16+0+(instr-1)*4,240+lvlid*4] = waveform + volume*16
			dest_pixels[3*16+1+(instr-1)*4,240+lvlid*4] = decay + sustain*16


	for n,tile_id in enumerate(indices) : 
		if tile_id > 0 : 
			x,y=n%256,n//256
			dest_pixels[x,y] = tile_id-1
			# reserve/check data in minimap 
			assert minimap[y//16][x//16] in (TRANSP,levelc),'conflict in minimap for level %s in (%d,%d) : had %d' %(lvlname,x//16,y//16,minimap[y//16][x//16])

			minimap[y//16][x//16]=levelc

			# if it is a ref to player tile, set this as player ID
			if tile_id-1 in player_tiles and lvlid is not None: 
				properties[lvlid][1]=tile_id-1

		

# export minimap
# -----------------------------------------------------
for ym,l in enumerate(minimap) : 
	for xm,c in enumerate(l) : 
		dest_pixels[xm,240+ym] = c

# export level / object properties
# ----------------------------------------------------
for ym,p in enumerate(properties) : 
	for xm,c in enumerate(p): 
		dest_pixels[16+xm+(ym//16)*8,240+ym%16] = c





# XXX levels 
# XXX terrains
# XXX checks pa overdraw ...

# finally save it as bmp 256c, non compressed
outfilename = args.file.rsplit('.',1)[0]+'.bmp'
dest_img.save(outfilename)
print "saved to %s"%outfilename

