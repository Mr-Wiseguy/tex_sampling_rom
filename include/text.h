#ifndef __TEXT_H__
#define __TEXT_H__

#include <types.h>
#include <cstring>

void set_text_color(int r, int g, int b, int a);
void print_text(int x, int y, char const* text, int length);
void print_text_centered(int x, int y, char const* text, int length);

FORCEINLINE void print_text(int x, int y, char const* text)
{
    print_text(x, y, text, std::strlen(text));
}

FORCEINLINE void print_text_centered(int x, int y, char const* text)
{
    print_text_centered(x, y, text, std::strlen(text));
}

void draw_all_text();

void text_init(); // Called once on startup for the text system to do any one-time setup
void text_reset(); // Called at the end of the frame to throw out any undrawn text labels

#endif