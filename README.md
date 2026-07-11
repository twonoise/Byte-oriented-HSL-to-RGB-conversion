# Byte oriented HSL to RGB conversion

Integer converter with byte input and output
--------------------------------------------

Today's display monitors use RGB color model, and this determines that programmer select this notation as obvious for _storing_ color constants.

In order of trying to make applications's colors more precise, flexible, easier to define when more than just few colors are need, and due to eye health care reasons like high contrast modes; or, when not just storage, but processing or generating of colors is required; -- one earlier or later notices that RGB is fairly inconsistent for that, so it's time to next level of understanding, HSL (or HSV) color model.

It is essentially more convenient and predictable for color operations, like make it brighter or darker, or even invert luma (brightness) without color damage, so red stays red, not aqua as per RGB inversion. If you need a bunch of colors for your diagram, then the answer will be like `hsl-to-rgb(N, 192, 64)` for paper and `(N, 192, 192)` for screen; how will you do it with RGB? Same if you need say day-of-week-dependent accents for your web page.

Current display monitors can't operate with HSL color model yet. So, conversion is need.
Let's look at bare HSL (without various compensations, as seen below TODO). It have the property that it keeps full dynamic range, and thus color itself, during RGB->HSL->RGB conversion.

Sadly, these conversions are quite not simple nor obvious. There are many forms of these, more or less precise or fast. But none byte-oriented one I found so far. Most known converters uses either (360, 101, 101) integer range, or (1.0, 1.0, 1.0) float range, while byte-range like (256, 256, 256) integer range are most demanded, due to constants define, storage, and processing. It is not a big secret that say (1.0, 1.0, 1.0) float range can be easily recalculated as byte range, and most time it's done like that. But when we want higher efficiency, known converters should be re-thinked as integer math with byte-range both input and output. Bytes are guarantees that there is no over/undershoot possible, makes them a requirement for hi-rel apps.

The problem with integer math is it way harder than floats. Here I am try to see if it will work, and how fast and precise it is.

I will use this reference code [^1]; this one, as well as any other you may find, are uses float math. This time it will be only one way, HSL->RGB. This solves most of tasks; not solved ones are include luma inversion of images (icons). TODO.

I provide C code here for research of precision and speed.

Current research results are show that there are two adequate solutions were made. One is high precision, and another one is somewhat faster. Please see `hsl2rgb` and `hsl2rgb_fast` at C code.

High precision one gives error no more than 1 for RGB values, compared to reference.
Faster one gives error of no more than 3, and about a tenth faster than precision one: `0.61 s` vs `0.67 s` per 100M calls using old core i3 CPU. While reference float math takes `~1.1 s` for that.

> These values are for **`-O2`** `gcc` or `clang` option. Using `-O3` may meet this issue[^3].

One may note that error of 3 is highly negligible, as it starts to occur from 1/3 of brightness (here error will be 1), which is far not same as this error near zero brightness, at today's gamma correction curve (near linear) of modern display units. Btw, one may also note that w3c online checker [^2] is also differs from reference.

Example: (here is the first brightness-sorted value with error of 1 occurs)

    a.out 00f42e

says

    HSL 00f42e, w3c hsl(0, 95%, 18%),  ref RGB 5a0101,  my RGB 590202, my RGBfast 590302

while use [^2] tool shows for _hsl(0, 95%, 18%)_

    #5A0202

Note that precision of w3c color model itself is low, due to it uses range of only `101` for `S` & `L`.

Both my `hsl2rgb` and `hsl2rgb_fast` are perfect candidates to be implemented with hardware, at display scaler IC, for future HSL display monitors. HSL on display cable is not have any disadvantage. Ask me if one need a 10-bit version for that.

I hope the C code i provide, is self-explaining as much as possible, and can be useful. Please let me know if it can be made faster or better.

[^1]: https://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
[^2]: https://www.w3schools.com/tools/tool_color_converter.php
[^3]: https://gist.github.com/twonoise/940c979ad3d0d9fc59fdefa5edd82a08
