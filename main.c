#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h> 
#include <time.h> 
#include <string.h> 

int game_over;
const int max_width = 20;
const int max_height = 20;
int head_x, head_y, apple_x, apple_y, current_score;
int body_x[400], body_y[400];
int tail_length;
int direction; 

// special item stuff
int is_item_spawned = 0; 
int item_x, item_y;
char item_symbol;         
int item_life = 0;  
int drunk_time_left = 0;    
int game_speed = 100;  
int fast_time_left = 0;    
int got_totem = 0;      

int difficulty_speed = 100;
int score_mult = 1; // Multiplier based on difficulty
char player_name[16] = "Player";

// struct for leaderboard
typedef struct {
    char name[16];
    int score;
} PlayerData;

PlayerData top_players[5]; 

// linking our assembly functions
extern void start_random_asm(int seed);
extern int get_random_num_asm(int max_val);
extern int calc_score_asm(int base_points, int is_sugar_active, int snake_length);

// moving cursor instead of wiping screen so it doesnt blink
void clear_screen() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD Position = {0, 0};
    SetConsoleCursorPosition(hOut, Position);
}

// hiding that annoying blinking text cursor
void hide_cursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE; 
    SetConsoleCursorInfo(consoleHandle, &info);
}

void print_spaces(int spaces) {
    for (int i = 0; i < spaces; i++) printf(" ");
}

// taking name input without breaking the terminal
void take_player_name(char* buffer, int limit) {
    int i = 0;
    while(1) {
        char ch = _getch();
        if (ch == '\r') { 
            if (i > 0) break; // need at least one letter
        } 
        else if (ch == '\b' && i > 0) { 
            printf("\b \b"); // visually delete it
            i--;         
        } 
        else if (i < limit - 1 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))) {
            printf("%c", ch); 
            buffer[i++] = ch; 
        }
    }
    buffer[i] = '\0'; 
}

// updating the leaderboard array
void save_score(char* p_name, int final_points) {
    for(int i = 0; i < 5; i++) {
        if(final_points >= top_players[i].score) {
            for(int j = 4; j > i; j--) {
                top_players[j] = top_players[j-1]; // push everyone down
            }
            strcpy(top_players[i].name, p_name);
            top_players[i].score = final_points;
            break; 
        }
    }
}

void show_leaderboard() {
    system("cls"); 
    printf("==============================\n");
    printf("     TOP 5 SESSION SCORES     \n");
    printf("==============================\n");

    int found_any = 0;
    for(int i = 0; i < 5; i++) {
        if(top_players[i].score > 0) {
            printf(" %d. %-15s : %d\n", i+1, top_players[i].name, top_players[i].score);
            found_any = 1;
        }
    }
    if (!found_any) printf(" No scores yet!\n");
    
    printf("==============================\n");
    printf("\nPress any key to return...");
    _getch();
}

void show_rules() {
    system("cls");
    printf("====================================================\n");
    printf("                   HOW TO PLAY                      \n");
    printf("====================================================\n");
    printf(" Use W, A, S, D to move. Avoid the walls!\n");
    printf(" Harder difficulty = Higher score multiplier!\n\n");
    printf(" --- SPECIAL ITEMS (Despawn after 3, 5, or 7 sec) ---\n");
    printf(" [ @ ] Apple      : Base Points. Grows snake.\n");
    printf(" [ $ ] Sugar Rush : Huge Points. 2x Speed!\n");
    printf(" [ %% ] Drunk Mode : Huge Points. Controls reverse!\n");
    printf(" [ T ] Totem      : ULTRA RARE. Cheats Death!\n");
    printf("      -> If you crash with a Totem, you can choose:\n");
    printf("         1. Resurrect (Lose half your score & size)\n");
    printf("         2. Cash Out  (Get a massive +50%% Score Bonus!)\