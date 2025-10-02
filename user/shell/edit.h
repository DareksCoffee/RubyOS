#ifndef __EDIT_h
#define __EDIT_h


void cmd_edit(const char* args);
int is_editmode();
void set_editmode(int mode);
void edit_input_char(char c);
void redraw_edit();
void edit_move_cursor_left();
void edit_move_cursor_right();

extern char current_file[];
extern int buffer_pos;
extern char edit_buffer[];
extern int cursor_pos;


#endif