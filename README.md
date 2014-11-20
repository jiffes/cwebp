cwebp
=====

another libwebp wrapper for Python with ctypes

<code>
	from ctypes import *
	webp = CCDL('cwebp.dylib')
	webp.jpeg2webp('a.jpg','b.webp')
</code>
