# make font
from PIL import Image
img=Image.open('4x6.png')
imgdata = img.load()
letters_perline = img.size[0]/4
nb_letters = img.size[1]/6 * letters_perline;
white=imgdata[0,0]

print "#include <stdint.h>"

print "const uint32_t font4x6[][6]={"
for letter in range( nb_letters ) :
    print '//',letter, chr(letter+32) if chr(letter+32) != '\\' else '<backslash>'
    print '{'
    for line in range(6) :
        l= [
            '00' if imgdata[letter%letters_perline*4 + x ,letter//letters_perline*6 + line]==white else '94'
            for x in range(4)
           ]
        print '0x'+''.join(l[::-1])+','
    print '},'
print "};"            
