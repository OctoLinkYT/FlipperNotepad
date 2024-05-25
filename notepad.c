#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>
#include <storage/storage.h>

#define MAX_FILENAME_LENGTH 32
#define MAX_TEXT_LENGTH 256
#define MAX_FILES 10
#define TEXT_FOLDER "/notepad/"

typedef enum {
    Mode_InitialMenu,
    Mode_Menu,
    Mode_Cursor,
    Mode_Edit,
} AppMode;

typedef struct {
    AppMode mode;
    char filename[MAX_FILENAME_LENGTH];
    char text[MAX_TEXT_LENGTH];
    int cursor_pos;
    int text_len;
    int selected_key;
    int selected_file;
    int num_files;
    char files[MAX_FILES][MAX_FILENAME_LENGTH];
    bool need_redraw;
} NotepadApp;

static const char* keyboard = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";

// Load existing files from the designated folder
void load_file_list(NotepadApp* app) {
    DIR* dir;
    struct dirent* entry;

    app->num_files = 0;
    dir = opendir(TEXT_FOLDER);
    if(dir != NULL) {
        while((entry = readdir(dir)) != NULL && app->num_files < MAX_FILES) {
            if(entry->d_type == DT_REG) {
                strncpy(app->files[app->num_files], entry->d_name, MAX_FILENAME_LENGTH - 1);
                app->files[app->num_files][MAX_FILENAME_LENGTH - 1] = '\0';
                app->num_files++;
            }
        }
        closedir(dir);
    }
}

// Load a file's content
void load_file(NotepadApp* app, const char* filename) {
    char filepath[MAX_FILENAME_LENGTH + strlen(TEXT_FOLDER)];
    snprintf(filepath, sizeof(filepath), "%s%s", TEXT_FOLDER, filename);

    FILE* file = fopen(filepath, "r");
    if(file) {
        app->text_len = fread(app->text, 1, MAX_TEXT_LENGTH - 1, file);
        app->text[app->text_len] = '\0';
        fclose(file);
    } else {
        app->text_len = 0;
        app->text[0] = '\0';
    }
    app->cursor_pos = 0;
}

// Save the current content to a file
void save_file(NotepadApp* app, const char* filename) {
    char filepath[MAX_FILENAME_LENGTH + strlen(TEXT_FOLDER)];
    snprintf(filepath, sizeof(filepath), "%s%s", TEXT_FOLDER, filename);

    FILE* file = fopen(filepath, "w");
    if(file) {
        fwrite(app->text, 1, app->text_len, file);
        fclose(file);
    }
}

// Render the initial menu (load or create file)
void render_initial_menu(Canvas* canvas, NotepadApp* app) {
    canvas_clear(canvas);
    const char* options[] = {"Load File", "Create New File"};
    for(int i = 0; i < 2; i++) {
        if(i == app->selected_file) {
            canvas_draw_box(canvas, 0, i * 10, 128, 10);
            canvas_set_color(canvas, Color_White);
        } else {
            canvas_set_color(canvas, Color_Black);
        }
        canvas_draw_str(canvas, 5, (i + 1) * 10, options[i]);
    }
    canvas_set_color(canvas, Color_Black);
    canvas_update(canvas);
}

// Render the file selection menu
void render_menu(Canvas* canvas, NotepadApp* app) {
    canvas_clear(canvas);
    for(int i = 0; i < app->num_files; i++) {
        if(i == app->selected_file) {
            canvas_draw_box(canvas, 0, i * 10, 128, 10);
            canvas_set_color(canvas, Color_White);
        } else {
            canvas_set_color(canvas, Color_Black);
        }
        canvas_draw_str(canvas, 5, (i + 1) * 10, app->files[i]);
    }
    canvas_set_color(canvas, Color_Black);
    canvas_update(canvas);
}

// Render the text content with a cursor
void render_text(Canvas* canvas, NotepadApp* app) {
    canvas_clear(canvas);
    canvas_draw_str(canvas, 5, 10, app->text);

    // Draw cursor
    int cursor_x = 5 + (app->cursor_pos % 20) * 6;
    int cursor_y = 10 + (app->cursor_pos / 20) * 10;
    canvas_draw_box(canvas, cursor_x, cursor_y - 8, 6, 10);

    canvas_update(canvas);
}

// Render the on-screen keyboard
void render_keyboard(Canvas* canvas, NotepadApp* app) {
    canvas_clear(canvas);
    for(int i = 0; i < strlen(keyboard); i++) {
        int x = (i % 10) * 12;
        int y = (i / 10) * 10;
        if(i == app->selected_key) {
            canvas_draw_box(canvas, x, y, 12, 10);
            canvas_set_color(canvas, Color_White);
        } else {
            canvas_set_color(canvas, Color_Black);
        }
        char str[2] = {keyboard[i], '\0'};
        canvas_draw_str(canvas, x + 3, y + 8, str);
    }
    canvas_set_color(canvas, Color_Black);
    canvas_update(canvas);
}

void input_callback(InputEvent* input_event, void* ctx) {
    NotepadApp* app = ctx;

    if(input_event->type == InputTypePress || input_event->type == InputTypeRepeat) {
        if(app->mode == Mode_InitialMenu) {
            if(input_event->key == InputKeyDown) {
                app->selected_file = (app->selected_file + 1) % 2;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyUp) {
                app->selected_file = (app->selected_file - 1 + 2) % 2;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyOk) {
                if(app->selected_file == 0) {
                    app->mode = Mode_Menu;
                    load_file_list(app);
                } else if(app->selected_file == 1) {
                    snprintf(app->filename, MAX_FILENAME_LENGTH, "newfile%d.txt", app->num_files + 1);
                    app->text_len = 0;
                    app->text[0] = '\0';
                    app->cursor_pos = 0;
                    app->mode = Mode_Cursor;
                }
                app->need_redraw = true;
            }
        } else if(app->mode == Mode_Menu) {
            if(input_event->key == InputKeyDown) {
                app->selected_file = (app->selected_file + 1) % app->num_files;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyUp) {
                app->selected_file = (app->selected_file - 1 + app->num_files) % app->num_files;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyOk) {
                strncpy(app->filename, app->files[app->selected_file], MAX_FILENAME_LENGTH);
                load_file(app, app->filename);
                app->mode = Mode_Cursor;
                app->need_redraw = true;
            }
        } else if(app->mode == Mode_Cursor) {
            if(input_event->key == InputKeyDown) {
                if(app->cursor_pos + 20 < app->text_len) app->cursor_pos += 20;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyUp) {
                if(app->cursor_pos - 20 >= 0) app->cursor_pos -= 20;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyLeft) {
                if(app->cursor_pos > 0) app->cursor_pos--;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyRight) {
                if(app->cursor_pos < app->text_len) app->cursor_pos++;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyOk) {
                app->mode = Mode_Edit;
                app->need_redraw = true;
            } else if(input_event->key == InputKeyBack) {
                app->mode = Mode_InitialMenu;
                save_file(app, app->filename);
                app->need_redraw = true;
            }
        } else if(app->mode == Mode_Edit) {
            if(input_event->key == InputKeyDown) {
                app->selected_key = (app->selected_key + 10) % strlen(keyboard);
                app->need_redraw = true;
            } else if(input_event->key == InputKeyUp) {
                app->selected_key = (app->selected_key - 10 + strlen(keyboard)) % strlen(keyboard);
                app->need_redraw = true;
            } else if(input_event->key == InputKeyLeft) {
                app->selected_key = (app->selected_key - 1 + strlen(keyboard)) % strlen(keyboard);
                app->need_redraw = true;
            } else if(input_event->key == InputKeyRight) {
                app->selected_key = (app->selected_key + 1) % strlen(keyboard);
                app->need_redraw = true;
            } else if(input_event->key == InputKeyOk) {
                if(app->text_len < MAX_TEXT_LENGTH - 1) {
                    memmove(&app->text[app->cursor_pos + 1], &app->text[app->cursor_pos], app->text_len - app->cursor_pos);
                    app->text[app->cursor_pos] = keyboard[app->selected_key];
                    app->text_len++;
                    app->cursor_pos++;
                }
                app->need_redraw = true;
            } else if(input_event->key == InputKeyBack) {
                app->mode = Mode_Cursor;
                app->need_redraw = true;
            }
        }
    }
}

void app_init(NotepadApp* app) {
    app->mode = Mode_InitialMenu;
    app->cursor_pos = 0;
    app->text_len = 0;
    app->selected_key = 0;
    app->selected_file = 0;
    app->num_files = 0;
    app->need_redraw = true;
    mkdir(TEXT_FOLDER, 0777); // Ensure the folder exists
}

void app_render(Canvas* canvas, void* ctx) {
    NotepadApp* app = ctx;

    if(app->need_redraw) {
        if(app->mode == Mode_InitialMenu) {
            render_initial_menu(canvas, app);
        } else if(app->mode == Mode_Menu) {
            render_menu(canvas, app);
        } else if(app->mode == Mode_Cursor) {
            render_text(canvas, app);
        } else if(app->mode == Mode_Edit) {
            render_keyboard(canvas, app);
        }
        app->need_redraw = false;
    }
}

int main(void) {
    NotepadApp app;
    app_init(&app);

    ViewPort* viewport = viewport_alloc();
    viewport_set_callback(viewport, app_render, &app);
    input_set_callback(input_alloc(), input_callback, &app);

    while(true) {
        furi_hal_power_suppress_charge_enter();
        furi_thread_yield();
    }

    return 0;
}
