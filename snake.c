
#include <pthread.h>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define SLEEP(ml) Sleep(ml)
    #define CLEAR_SCREEN system("cls")
#elif defined(__linux__)
    #include <unistd.h>
    #define SLEEP(ml) sleep(ml)
    #define CLEAR_SCREEN system("clear")
#else
    #error Your OS is not supported
#endif

#define COLUMNS 50
#define ROWS    20
#define W 'w'
#define A 'a'
#define D 'd'
#define S 's'
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4



struct snake_position {
    int x;
    int y;
};

struct ktoi_struct {
    int value;
    char key;
};

struct snake_segment {
    int prev_x;
    int prev_y;
    char body;
    struct snake_position position;
};

typedef struct {
    char body;
    struct snake_position position;
    struct snake_segment * segments;;
} snake;

void create_window();
void create_snake();
void * get_input(void);
int ktoi(char key_pressed);
void update_direction();
void generate_food();
void remove_food();
void eat_food(int x, int y);
void new_segment(int x, int y);
void move_segments(int x, int y);

snake snake_;
WINDOW * snake_box;
int IS_OVER = 0, EXIT_FLAG = 0, food_x, food_y, food_timer = 4, last_direction, segments_size = 0, speed = 50, points = 0, IS_SPECIAL = 0;
clock_t start_timer;
char input;

int main() {
    initscr();
    cbreak();
    noecho();
    create_window();
    create_snake();
    start_color();
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, get_input, NULL);
    attron(COLOR_RED);
    mvwprintw(snake_box, 0, 5, "Score: ");
    while(!IS_OVER && !EXIT_FLAG) {
        int last_x = snake_.position.x, last_y = snake_.position.y;
        update_direction(last_x, last_y);
        if(start_timer != NULL && (((clock() - start_timer) * 1000)/CLOCKS_PER_SEC)/1000 >= food_timer) {
            remove_food();
        } else if (start_timer == NULL) {
            generate_food();
        }
        if(IS_OVER == 1) {
            start_color();
            init_color(1, COLOR_RED, COLOR_BLUE, 10);
            attron(COLOR_PAIR(1));
            mvwprintw(snake_box, 1, 20, "Game Over!");
            attroff(COLOR_PAIR(1));
            wrefresh(snake_box);
            SLEEP(2000);
            break;
        }
        eat_food(last_x, last_y);
        wrefresh(snake_box);
        SLEEP(speed);
    }
    endwin();
    s_gc();

    return 0;
}

void s_gc() {
    free(snake_.segments);
}

void create_window() {
    snake_box = newwin(ROWS, COLUMNS, 5, 30);
    box(snake_box, 0, 0);
}

void create_snake() {
    if((snake_.segments = malloc(sizeof(struct snake_segment) * (ROWS * COLUMNS))) != NULL) {
        snake_.position.x = (COLUMNS/2)-2;
        snake_.position.y = (ROWS/2)-1;
        snake_.body = 'X';
    }
}

void * get_input() {
    static char keys[] = {A, D, W, S};
    while(!IS_OVER && !EXIT_FLAG) {
        if(_kbhit()) {
            char validate_input = wgetch(snake_box);
            if(validate_input == 'x') {
                EXIT_FLAG = 1;
            } else if(
                      (validate_input == W && last_direction == DOWN) ||
                      (validate_input == S && last_direction == UP) ||
                      (validate_input == A && last_direction == RIGHT) ||
                      (validate_input == D && last_direction == LEFT)
                      ) {
                        continue;
            } else {
                for(int i = 0; i < 4; ++i) {
                    if(validate_input == keys[i]) {
                        input = validate_input;
                        break;
                    }
                }
            }
        }
    }
}

int ktoi(char key_pressed) {
    static struct ktoi_struct key_lookup[] = {
        {UP, W}, {DOWN, S}, {LEFT, A}, {RIGHT, D}
    };
    for(int i = 0; i < 4; ++i) {
        if(key_pressed == key_lookup[i].key) {
            return key_lookup[i].value;
        }
    }
    return -1;
}

void update_direction(int last_x, int last_y) {
    switch(ktoi(input)){
        case UP:
            snake_.position.y -= 1;
            last_direction = UP;
            break;
        case DOWN:
            snake_.position.y += 1;
            last_direction = DOWN;
            break;
        case LEFT:
            snake_.position.x -= 1;
            last_direction = LEFT;
            break;
        case RIGHT:
            snake_.position.x += 1;
            last_direction = RIGHT;
            break;
    }
    if(snake_.position.x + 1 == COLUMNS) {
        snake_.position.x = 1;
    } else if(snake_.position.x == 0) {
        snake_.position.x = COLUMNS - 2;
    } else if(snake_.position.y + 1 == ROWS) {
        snake_.position.y = 1;
    } else if(snake_.position.y  == 0) {
        snake_.position.y = ROWS - 2;
    } else if((mvwinch(snake_box, snake_.position.y, snake_.position.x) & A_CHARTEXT) == 'o') {
        IS_OVER = 1;
        return;
    }
    mvwaddch(snake_box, snake_.position.y, snake_.position.x, snake_.body);
    mvwaddch(snake_box, last_y, last_x, ' ');
    move_segments(last_x, last_y);
}

void generate_food() {
    while(1) {
        food_x = random_number(0, COLUMNS - 2), food_y = random_number(0, ROWS - 2);
        if((mvwinch(snake_box, food_y, food_x) & A_CHARTEXT) != 'o' || (mvwinch(snake_box, food_y, food_x) & A_CHARTEXT) != 'o') {
            char food = 'F';
            if(random_number(0, 10) == 1) {
                IS_SPECIAL = 1;
                food = 'S';
            } else if(IS_SPECIAL) {
                IS_SPECIAL = 0;
            }
            mvwaddch(snake_box, food_y, food_x, food);
            start_timer = clock();
            break;
        }
    }
}

void remove_food() {
    mvwaddch(snake_box, food_y, food_x, ' ');
    start_timer = NULL;
}

int random_number(int min, int max) {
    return rand() % ((max + 1) - min) + min;
}

void eat_food(int x, int y) {
    if(x == food_x && y == food_y) {
        switch(IS_SPECIAL){
            case 0:
                points += 1;
                break;
            case 1:
                points += 2;
                break;
        }
        if(points % 10 == 0) {
            speed -= 20;
        }
        new_segment(x, y);
    }
}

void new_segment(int x, int y) {
    struct snake_segment segment;
    segment.body = 'o';
    if(segments_size == 0) {
        set_coords(&segment, x, y);
    } else {
        struct snake_segment parent = snake_.segments[segments_size - 1];
        set_coords(&segment, parent.prev_x, parent.prev_y);
    }
    snake_.segments[segments_size] = segment;
    segments_size += 1;
    mvwaddch(snake_box, segment.position.y, segment.position.x, segment.body);
    if(IS_SPECIAL) {
        struct snake_segment parent = snake_.segments[segments_size - 1];
        set_coords(&segment, parent.prev_x, parent.prev_y);
        snake_.segments[segments_size] = segment;
        segments_size += 1;
        mvwaddch(snake_box, segment.position.y, segment.position.x, segment.body);
    }
}

void move_segments(int x, int y){
    for(int i = 0; i <= segments_size - 1; ++i) {
        struct snake_segment segment = snake_.segments[i];
        segment.prev_x = segment.position.x;
        segment.prev_y = segment.position.y;
        if(i == 0) {
            set_coords(&segment, x, y);
        } else {
            struct snake_segment parent_ = snake_.segments[i - 1];
            set_coords(&segment, parent_.prev_x, parent_.prev_y);
        }
        snake_.segments[i] = segment;
        mvwaddch(snake_box, segment.position.y, segment.position.x, segment.body);
        mvwaddch(snake_box, segment.prev_y, segment.prev_x, ' ');
    }
}

void set_coords(struct snake_segment * target, int x, int y) {
    target->position.x = x;
    target->position.y = y;
}

