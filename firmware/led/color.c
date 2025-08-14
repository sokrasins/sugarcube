#include "color.h"
#include <math.h>

void color_brightness_set(color_t *color, float brightness)
{
    color->r = color->r * brightness;
    color->g = color->g * brightness;
    color->b = color->b * brightness;
}

void color_interp(const color_t *color1, const color_t *color2, float point, color_t *out)
{
    out->r = color1->r + ((color2->r - color1->r) * point);
    out->g = color1->g + ((color2->g - color1->g) * point);
    out->b = color1->b + ((color2->b - color1->b) * point);
}