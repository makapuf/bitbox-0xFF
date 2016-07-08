#!/usr/bin/python2
from PIL import Image 
import sys, argparse
import xml.etree.ElementTree as ET
import array, os.path, argparse

sys.argv.append('assets/chateau.tmx')

parser = argparse.ArgumentParser(description='Process TMX files to tset/tmap/.h files')
parser.add_argument('file',help='input .tmx filename')
args = parser.parse_args()

PALETTE = os.path.dirname(__file__)+'/pal_micro.png'

tree = ET.parse(args.file)
root = tree.getroot()

minimap = [[0]*16]*16

# tileset
# -----------------------------------------------------

# open tileset as the basis of the output file.
# tileset must be 256x256. transparent is color 230, #4800da
tileset = root.find('tileset') # only one tileset, 16x16 
assert tileset.get("tilewidth")==tileset.get("tileheight")=='16', "tiles must be 16x16"
assert tileset.get("tilecount")=='256',"tileset must be 256x256 (16 by 16 tiles)"

tileset_imagename = os.path.join(os.path.dirname(os.path.abspath(args.file)),tileset.find("image").get("source"))
tileset_image=Image.open(tileset_imagename)

assert tileset_image.size == (256,256)
assert tileset_image.mode == 'P','image must be 256 colors'

# make a copy for resulting image
dest_img = tileset_image.copy()
dest_pixels = dest_img.load()

# XXX reserve tileset data in minimap if there is one non-empty pixel


# export tilemap to image
# -----------------------------------------------------

tilemap = root.find("layer")
assert tilemap.get("width")  == tilemap.get("height") == '256','tilemap must be 256x176'

# XXX check data in minimap

data = tilemap.find("data")
if data.get('encoding')=='csv' :
    indices = [int(s) for s in data.text.replace("\n",'').split(',')]
elif data.get('encoding')=='base64' and data.get('compression')=='zlib' :
    indices = array.array('I',data.text.decode('base64').decode('zlib'))
else :
    raise ValueError,'Unsupported layer encoding :'+data.get('encoding')

for n,tile_id in enumerate(indices) : 
	# XXX reserve data in minimap ? / check minimap
	if tile_id > 0 : 
		dest_pixels[n%256,n//256] = tile_id-1

# export minimap
# -----------------------------------------------------
for ym,l in enumerate(minimap) : 
	for xm,c in enumerate(l) : 
		dest_pixels[xm,240+ym] = c



# finally save it as bmp 256c, non compressed
outfilename = args.file.rsplit('.',1)[0]+'_out.bmp'
dest_img.save(outfilename)
print "saved to %s"%outfilename

