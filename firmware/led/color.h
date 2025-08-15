#ifndef COLOR_H_
#define COLOR_H_

typedef struct {
 float r;
 float g;
 float b;
} color_t;

// Pre-defined colors
static const color_t COLOR_RED    = { 1.0, 0.0, 0.0 };
static const color_t COLOR_YELLOW = { 1.0, 1.0, 0.0 };
static const color_t COLOR_GREEN  = { 0.0, 1.0, 0.0 };
static const color_t COLOR_CYAN   = { 0.0, 1.0, 1.0 };
static const color_t COLOR_BLUE   = { 0.0, 0.0, 1.0 };
static const color_t COLOR_PURPLE = { 1.0, 0.0, 1.0 };
static const color_t COLOR_WHITE  = { 1.0, 1.0, 1.0 };
static const color_t COLOR_BLACK  = { 0.0, 0.0, 0.0 };

void color_brightness_set(color_t *color, float brightness);

void color_interp(const color_t *color1, const color_t *color2, float point, color_t *out);

#endif /*COLOR_H_*/