from ctypes import *
webp = CCDL('cwebp.dylib')
webp.jpeg2webp('a.jpg','b.webp')
