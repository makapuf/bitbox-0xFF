#!/usr/bin/python2
"""

Representation of a level in TMX 
- tilesets are defined as one .png file. copies of that png can be defined after for objects with different tilesizes (always multiples of tilesize)
- terrains are defined in the tileset and are labelled obstacle, ice, ladder, platform, ...

- tilemap is made of at least 5 levels, named level1-4 & intro (which must be at the right place )
  when generating from level, two guidelines are defined : a checkerboard made of tiles 0 and 1 and a grid made of the tiles themselves for reference of where they are defined and where they are free. 230-defined colors are left empty 
- additional levels should be prefixed with _ to not count as a level
- object types are properties of their first tile, which must be defined with properties "movement","collision","spawn". They are placed on the map as tiles.
- player movement type is "player"
- music tile should be placed at right position in a 16x16 pattern with references to music patterns
- level properties are defined with its layer, including music (see after)
- music instrs are defined as properties of a level (volume0..3 = 0..15, waveform0..3= )

TODO : 
- music pattern with .song file property ? 
- reference spawn by names
- move terrain to tile property

"""
from PIL import Image 
import sys, argparse
import xml.etree.ElementTree as ET
import array, os.path, argparse
from mk_defs import parse



TRANSP=230
PALETTE = os.path.dirname(__file__)+'/pal_micro.png'


def parse_defs(reverse=False) : 
    defs_sections={}
    for section_name, header, table, comments in parse(open('REFERENCE.md')) : 
        d={}
        defs_sections[section_name.lower()]=d
        for i in table: 
            name,id,cmt=list(i)
            if id : 
                try : 
                    id=int(id)
                    if reverse : 
                        d[name]=id
                    else : 
                        d[id]=name
                except ValueError,e : 
                    pass
    #for i in defs_sections.items() : print i
    return defs_sections



# parses minimap
def get_minimap(x,y) : 
    return pixels[x, (15*16+y)]

def parse_minimap() :
    # return layer_id of each tile
    levelcolors=[objtypes[i]['color'] for i in range(4)]
    print 'levelcolors', levelcolors
    # level of each tile 0=fixed/sprites, 1,2,3,4 : level N, 5 = system (will be regenerated, minimap/object types)
    minimap=[];tileprop={}
    terrain_names = list(set(defs['terrains'].values()))
    for y in range(16) : 
        minimap.append([])
        for x in range(16) : 
            lvlc=get_minimap(x,y)
            if lvlc in levelcolors and lvlc != TRANSP: 
                layer=levelcolors.index(lvlc)+1
            else : 
                layer=0
                if lvlc in defs['terrains']: 
                    tn=defs['terrains'][lvlc]
                    tileprop[y*16+x]=terrain_names.index(tn)
            minimap[-1].append(layer) 
    lastline=minimap[15]
    lastline[0]=-1
    lastline[1]=0 # TODO ! now set to -1 
    lastline[2]=5 
    return minimap,tileprop,terrain_names

def parse_objtypes() : 
    def get_property(id,property) : 
        return pixels[16+property+(id//16)*8,15*16+id%16]

    return [ {
            k:get_property(i,n) for n,k in defs['levels' if i<4 else 'object types'].items()
            } for i in range(32) ]

def gen_tilesetimg() : 
    "uses minimap, src"
    outimg=src.copy()
    outpix=outimg.load()
    
    def delete_rect(x,y) : 
        "deletes rectangle at x,y (in 0-16)"
        for dy in range(16) : 
            for dx in range(16) : 
                outpix[x*16+dx,y*16+dy]=TRANSP

    # blanks non level0 tiles
    for y in range(16) : 
        for x in range(16):  
            if minimap[y][x]!=0 : 
                delete_rect(x,y)

    return outimg


def gen_tmx(tileset,tilemap) :
    with open(tilemap,'w') as of :
        of.write('<map version="1.0" orientation="orthogonal" width="256" height="256" tilewidth="16" tileheight="16">\n')
        of.write ('<tileset firstgid="1" name="tilemap" tilewidth="16" tileheight="16" tilecount="256" spacing="0" margin="0" >\n')
        of.write ('    <image source="%s"/>\n'%tileset)
        of.write('<terraintypes>')
        for tn in terrain_names : 
            of.write('<terrain name="%s" tile="-1"/>'%tn)
        of.write('</terraintypes>')

        for k,v in tileterrains.items() : 
            of.write('<tile id="%d" terrain="%d,%d,%d,%d"/>'%(k,v,v,v,v))
        of.write ('</tileset>\n')
        for layer in 5,1,2,3,4 : 
            layername = "level %d"%layer if layer!=5 else 'intro'
            of.write ('<layer name="%s" width="256" height="256" >'%layername)
            of.write ('<data encoding="csv">')
            for i in range(65536) : 
                x,y=i%256 , i//256
                tx,dx=x//16, x%16
                ty,dy=y//16, y%16
                of.write(str((pixels[x,y]+1) if minimap[ty][tx]==layer else 0)) # str()
                if i!=65535 : of.write(',')
            of.write ('</data>\n</layer>')

        # checkerboard grid
        of.write ('<layer name="_grid" width="256" height="256" opacity="0.2">')
        of.write ('<data encoding="csv">')
        for i in range(65536) : 
            x,y=i%256, i//256
            of.write(str(1 if (x//16+y//16)%2==0 else 2))
            if i!=65535 : of.write(',')
        of.write ('</data>\n</layer>')

        # pixel grid
        of.write ('<layer name="_pixels" width="256" height="256" opacity="0.2">')
        of.write ('<data encoding="csv">')
        for i in range(65536) : 
            x,y=i%256, i//256
            c = pixels[x,y]
            of.write(str(c+1 if c != 230 else 0))
            if i!=65535 : of.write(',')
        of.write ('</data>\n</layer>')


        of.write('\n</map>')

if __name__=='__main__' : 
    argparser = argparse.ArgumentParser(description='process level to generate tmx/song files')
    argparser.add_argument('file',nargs='?',help='input .bmp filename',default='levels/level0.bmp')
    args = argparser.parse_args()

    src=Image.open(args.file)
    pixels=src.load()
    assert src.size == (256,256), "input file must be 256x256"

    defs=parse_defs()
    objtypes = parse_objtypes()
    minimap, tileterrains,terrain_names = parse_minimap()

    for n,k in enumerate(objtypes) : print n,k
    for m in minimap : print m
    print {k:terrain_names[v] for k,v in tileterrains.items()}
    gen_tilesetimg().save('out.png')
    gen_tmx('out.png','out.tmx')

    # objects+types : 1 sprite utilise forcement. ordre ! (pour switches)
    # level color
    # black mapper ?? simple export
    # exports dummy layer with tiles defined (either by terrain or with pixels on tileset)
    # ids in grid ??


