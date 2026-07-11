#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>       /* round() */
#include "sys/param.h"  /* MIN(), MAX() */

#define FIT(x, min, max) (x < min ? min : x > max ? max : x)
#define BYTE(x, n) (((uint8_t *)&x)[n])

/* Reference */
float hue2rgb(float p, float q, float t)
{
    if(t < 0.0f) t += 1.0f;
    if(t > 1.0f) t -= 1.0f;
    if(t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if(t < 1.0f/2.0f) return q;
    if(t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

uint32_t hslToRgb(uint8_t hh, uint8_t ss, uint8_t ll)
{
    float h = hh / 255.0f;
    float s = ss / 255.0f;
    float l = ll / 255.0f;

    float r, g, b;

    if (s == 0.0f)
        r = g = b = l; /* achromatic */
    else
    {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hue2rgb(p, q, h + 1.0f/3.0f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f/3.0f);
    }

    uint8_t rr = r * 255.0f;
    uint8_t gg = g * 255.0f;
    uint8_t bb = b * 255.0f;

    return (rr << 16) + (gg << 8) + bb;
}

/* My integer conversions. Can someone made it faster? */
uint32_t hsl2rgb(uint8_t h, uint8_t s, uint8_t l)  /* Full range 0..255 each */
{
    uint16_t A = s * MIN(l, 255 - l);

    #define k(N, h) (uint16_t) ((N * 255 + h * 12) % (12 * 255))
    #define f(N) (uint8_t) (((l << 16) - A * MAX(MIN(MIN(k(N, h) - 3*255, 9*255 - k(N, h)), 256), -256)) >> 16)

    /* Or just (((f(0) << 8) + f(8)) << 8) + f(4) will work */
    return s ? (((f(0) << 8) + f(8)) << 8) + f(4) : (l << 16) + (l << 8) + l;
}

uint32_t hsl2rgb_fast(uint8_t h, uint8_t s, uint8_t l)  /* Full range 0..255 each */
{
    uint16_t A = s * MIN(l, 255 - l);

    /* Note that 49152 can't be adjusted, as it drops CPU time gain we won */
    #define kf(N, h) (uint16_t) (((N << 12) + h * 192) % 49152)  /* 17 bits used */
    #define ff(N) (uint8_t) (((l << 20) - A * MAX(MIN(MIN(kf(N, h) - 3*256*16+111, 9*256*16-81 - kf(N, h)), 256*16), -256*16)) >> 20)

    return s ? (((ff(0) << 8) + ff(8)) << 8) + ff(4) : (l << 16) + (l << 8) + l;
}

/* TODO Also check this https://stackoverflow.com/a/42261473/5540531
vec3 hsl2rgb( vec3 c ) {
    vec3 rgb = clamp(abs(mod(c.x*6.0 + vec3(0.0, 4.0, 2.0), 6.0)-3.0)-1.0, 0.0, 1.0);
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
} */


int main(int argc, char *argv[])
{
    uint32_t given = argc > 1 ? strtoul(argv[1], NULL, 16) : 0;

    if (given)
    {
        if (given >> 24) /* Pass -1 to measure time, like 'time a.out -1' */
        {
            uint32_t temp = 0; /* Don't let to optimize it out */
            for (int i = 0; i < 100000000; i++)
            {
                uint8_t r = i % 256; /* Somewhat like rand(), but faster */
                temp += hslToRgb(r, r, r); /* Select one */
                // temp += hsl2rgb(r, r, r); /* Select one */
                // temp += hsl2rgb_fast(r, r, r); /* Select one */
            }
            printf("%d calls: done. %d\n", 100000000, temp);

        }
        else
        {
            uint8_t h = BYTE(given, 2);
            uint8_t s = BYTE(given, 1);
            uint8_t l = BYTE(given, 0);

            uint32_t result_ref = hslToRgb(h, s, l);
            uint32_t result_my = hsl2rgb(h, s, l);
            uint32_t result_my_fast = hsl2rgb_fast(h, s, l);

            printf("HSL %02x%02x%02x, w3c hsl(%d, %d%%, %d%%),  ref RGB %06x,  my RGB %06x, my RGBfast %06x, w3c RGB please see its 'tool_color_converter.php'.\n", h, s, l, (int)round(h / 256.0 * 360.0), (int)round(s / 256.0 * 100.0), (int)round(l / 256.0 * 100.0), result_ref, result_my, result_my_fast);
        }
    }
    else
    {
        int errors = 0;
        int step = 1;   /* Use 2^N */

        for (int ll = 0; ll <= 256; ll = ll + step)
            for (int ss = 0; ss <= 256; ss = ss + step)
                for (int hh = 0; hh <= 256; hh = hh + step)
                {
                    uint8_t h = (uint8_t) FIT(hh > 127 ? hh - 1 : hh, 0, 255);
                    uint8_t s = (uint8_t) FIT(ss > 127 ? ss - 1 : ss, 0, 255);
                    uint8_t l = (uint8_t) FIT(ll > 127 ? ll - 1 : ll, 0, 255);

                    uint32_t result_ref = hslToRgb(h, s, l);
                    // uint32_t result_my = hsl2rgb(h, s, l);
                    uint32_t result_my = hsl2rgb_fast(h, s, l);

                    int deltaR = BYTE(result_ref, 2) - (int)BYTE(result_my, 2);
                    int deltaG = BYTE(result_ref, 1) - (int)BYTE(result_my, 1);
                    int deltaB = BYTE(result_ref, 0) - (int)BYTE(result_my, 0);

                    int max_delta = 3;  /* Use 3 for hsl2rgb_fast */

                    if ( (abs(deltaR) > max_delta) ||
                         (abs(deltaG) > max_delta) ||
                         (abs(deltaB) > max_delta) ||
                    /* Special care for black and white: no error allowed for them. */
                         ((result_ref == 0x000000) && (result_my != 0x000000)) ||
                         ((result_ref == 0xffffff) && (result_my != 0xffffff))
                       )
                    {
                        printf("Error: HSL %02x%02x%02x,  ref RGB %06x,  my RGB %06x\n",
                               h, s, l, result_ref, result_my);
                        if (errors++ > 8)
                        {
                            fprintf(stderr, "Too many errors, will quit.\n");
                            return 0;
                        }
                    }
                }

        if (! errors)
            printf("Test passed.\n");
    }

    return 0;
}
