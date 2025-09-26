#ifndef MOUSE_CURSOR_H
#define MOUSE_CURSOR_H

void init_cursor(void);
void update_cursor(int x, int y);
void render_cursor(void);
int get_cursor_x(void);
int get_cursor_y(void);

#endif