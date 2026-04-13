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
int score_mult = 1; //Changes the score if difficulty changes
char player_name[16] = "Player";

// leaderboard
typedef struct {
    char name[16];
    int score;
} PlayerData;

PlayerData top_players[5]; 

// linking assembly functions
extern void start_random_asm(int seed);
extern int get_random_num_asm(int max_val);
extern int calc_score_asm(int base_points, int is_sugar_active, int snake_length);

// the cursor clears everything to make sure nothing extra is visible on screen
void clearscreen() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD Position = {0, 0};
    SetConsoleCursorPosition(hOut, Position);
}

// hiding the cursor 
void hidecursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE; 
    SetConsoleCursorInfo(consoleHandle, &info);
}

void printspace(int spaces) {
    for (int i = 0; i < spaces; i++) printf(" ");
}

// name of player
void take_player_name(char* buffer, int limit) {
    int i = 0;
    while(1) {
        char ch = _getch();
        if (ch == '\r') { 
            if (i > 0) break; // need at least one letter
        } 
        else if (ch == '\b' && i > 0) { 
            printf("\b \b"); 
            i--;         
        } 
        else if (i < limit - 1 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))) {
            printf("%c", ch); 
            buffer[i++] = ch; 
        }
    }
    buffer[i] = '\0'; 
}

// updating the leaderboard 
void save_score(char* p_name, int final_points) {
    for(int i = 0; i < 5; i++) {
        if(final_points >= top_players[i].score) {
            for(int j = 4; j > i; j--) {
                top_players[j] = top_players[j-1]; 
            }
            strcpy(top_players[i].name, p_name);
            top_players[i].score = final_points;
            break; 
        }
    }
}
// function to show leaderboard
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
// screen comes up before the game starts to explain everything to new player
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
    printf("         2. Cash Out  (Get a massive +50%% Score Bonus!)\n\n");
    printf("====================================================\n");
    printf(" Press any key to start...\n");
    _getch(); 
}
// main menu of the game comes up when .exe files is run
void main_menu() {
    int in_menu = 1;
    while (in_menu) {
        system("cls"); 
        printf("==============================\n");
        printf("     TERMINAL SNAKE v1.0      \n");
        printf("==============================\n");
        printf("1. Start Game\n");
        printf("2. Change Difficulty (Current: %s)\n", difficulty_speed == 150 ? "Easy" : difficulty_speed == 100 ? "Normal" : "Hard");
        printf("3. View High Scores\n");
        printf("4. Exit\n");
        printf("==============================\n");
        printf("Select an option: ");

        char input = _getch(); 

        if (input == '1') {
            printf("\n\nEnter Player Name (Letters/Numbers only): ");
            take_player_name(player_name, 15);
            show_rules(); 
            in_menu = 0; 
        }
        else if (input == '2') {
            printf("\n\nSelect Difficulty:\n1. Easy (1x Pts)\n2. Normal (2x Pts)\n3. Hard (3x Pts)\n> ");
            char d = _getch();
            if (d == '1') difficulty_speed = 150;
            else if (d == '2') difficulty_speed = 100;
            else if (d == '3') difficulty_speed = 50;
        }
        else if (input == '3') show_leaderboard();
        else if (input == '4') {
            system("cls");
            exit(0);
        }
    }
    system("cls"); 
}
// reseting game variables before every new game
void newgame() {
    hidecursor();
    start_random_asm((int)time(NULL)); 
    
    game_over = 0;
    direction = 0;
    head_x = max_width / 2;
    head_y = max_height / 2;
    apple_x = get_random_num_asm(max_width);
    apple_y = get_random_num_asm(max_height);
    current_score = 0;
    tail_length = 0;
    
    drunk_time_left = 0;
    fast_time_left = 0;
    is_item_spawned = 0;
    got_totem = 0;
    game_speed = difficulty_speed; 
    
    // changing the multiplier accoring to difficulty
    if (difficulty_speed == 150) score_mult = 1;
    else if (difficulty_speed == 100) score_mult = 2;
    else if (difficulty_speed == 50) score_mult = 3;
}
// drawing the main grid where the snake moves
void grid() {
    clearscreen(); 

    // calculates the terminal windows size and shifts the grid to theh centre of terminal
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    int cols = info.srWindow.Right - info.srWindow.Left + 1;
    int rows = info.srWindow.Bottom - info.srWindow.Top + 1;

    int grid_chars = (max_width + 2) * 2;
    int pad_x = (cols - grid_chars) / 2;
    int pad_y = (rows - (max_height + 7)) / 2; 

    if (pad_x < 0) pad_x = 0;
    if (pad_y < 0) pad_y = 0;

    for (int i = 0; i < pad_y; i++) printf("\n");

    print_spaces(pad_x);
    for (int i = 0; i < max_width + 2; i++) printf("# ");
    printf("\n");

    for (int i = 0; i < max_height; i++) {
        print_spaces(pad_x);
        for (int j = 0; j < max_width; j++) {
            if (j == 0) printf("# "); 
                
            if (i == head_y && j == head_x) {
                char head = 'O'; 
                if (direction == 1) head = '<';
                else if (direction == 2) head = '>';
                else if (direction == 3) head = '^';
                else if (direction == 4) head = 'v';
                printf("%c ", head); 
            }
            else if (i == apple_y && j == apple_x)
                printf("@ "); 
            else if (is_item_spawned == 1 && i == item_y && j == item_x)
                printf("%c ", item_symbol); 
            else {
                int is_tail = 0;
                for (int k = 0; k < tail_length; k++) {
                    if (body_x[k] == j && body_y[k] == i) {
                        if (k == 0) printf("O "); // neck is capital
                        else printf("o "); 
                        is_tail = 1;
                        break; 
                    }
                }
                if (!is_tail) printf("  "); 
            }
                
            if (j == max_width - 1) printf("# "); 
        }
        printf("\n");
    }

    print_spaces(pad_x);
    for (int i = 0; i < max_width + 2; i++) printf("# ");
    printf("\n");
    
    print_spaces(pad_x); printf("Player: %s | Score: %d (x%d Multiplier)\n", player_name, current_score, score_mult);
    
    print_spaces(pad_x);
    if (got_totem) printf("[+] TOTEM EQUIPPED! Crash to use it!           \n");
    else if (drunk_time_left > 0) printf("[!] DRUNK MODE ACTIVE! Controls Reversed!    \n");
    else if (fast_time_left > 0) printf("[!] SUGAR RUSH! 2x Speed & 2x Points!        \n");
    else printf("Use WASD to move, X to quit.                 \n");
}
// to check the key inputs
void keyinputs() {
    if (_kbhit()) {
        char k = _getch();
        
        // changing the controls for special drunk mode
        if (drunk_time_left > 0) {
            if (k == 'w') k = 's';
            else if (k == 's') k = 'w';
            else if (k == 'a') k = 'd';
            else if (k == 'd') k = 'a';
        }

        switch (k) {
        case 'a': if (direction != 2) direction = 1; break;
        case 'd': if (direction != 1) direction = 2; break;
        case 'w': if (direction != 4) direction = 3; break;
        case 's': if (direction != 3) direction = 4; break;
        case 'x': game_over = 1; break;
        }
    }
}
// does the main work in game 
void maingame() {
    int p_x = body_x[0];
    int p_y = body_y[0];
    int p2_x, p2_y;
    body_x[0] = head_x;
    body_y[0] = head_y;
    
    // moving the body segments
    for (int i = 1; i < tail_length; i++) {
        p2_x = body_x[i];
        p2_y = body_y[i];
        body_x[i] = p_x;
        body_y[i] = p_y;
        p_x = p2_x;
        p_y = p2_y;
    }

    switch (direction) {
    case 1: head_x--; break; 
    case 2: head_x++; break; 
    case 3: head_y--; break; 
    case 4: head_y++; break; 
    default: break;
    }

    // check if we hit something
    int hit_wall = 0;
    if (head_x >= max_width || head_x < 0 || head_y >= max_height || head_y < 0) hit_wall = 1;
    
    // check self-collision if we are actually moving 
    if (direction != 0) {
        for (int i = 0; i < tail_length; i++)
            if (body_x[i] == head_x && body_y[i] == head_y) hit_wall = 1;
    }

    // totem ability if you die
    if (hit_wall) {
        if (got_totem) {
            system("cls");
            printf("\n\n=================================================\n");
            printf("               TOTEM ACTIVATED!                  \n");
            printf("=================================================\n");
            printf(" You crashed, but the Totem gives you a choice:\n\n");
            printf(" [1] RESURRECT : Keep playing (Lose half score & size)\n");
            printf(" [2] CASH OUT  : Accept death (Get a massive +50%% Score Bonus!)\n\n");
            printf(" Choose your fate (1 or 2): ");
            
            char choice;
            while(1) {
                choice = _getch();
                if (choice == '1' || choice == '2') break;
            }

            if (choice == '1') {
                // resurrected
                got_totem = 0;       
                tail_length /= 2;          
                current_score /= 2;        
                head_x = max_width / 2;    
                head_y = max_height / 2;
                direction = 0;             
                drunk_time_left = 0; 
                
                // move the body in safe position 
                for(int i = 0; i < tail_length; i++) {
                    body_x[i] = head_x;
                    body_y[i] = head_y;
                }
            } else {
                // adding 50% to the score
                current_score += (current_score / 2);
                game_over = 1;
            }
        } else {
            game_over = 1;
        }
    }

    if (drunk_time_left > 0) drunk_time_left--;
    if (fast_time_left > 0) {
        fast_time_left--;
        if (fast_time_left <= 0) game_speed = difficulty_speed; 
    }
    if (is_item_spawned == 1) {
        item_life--;
        if (item_life <= 0) is_item_spawned = 0; 
    }

    // ate normal apple 
    if (head_x == apple_x && head_y == apple_y) {
        int sugar_stat = (fast_time_left > 0) ? 1 : 0; 
        current_score += calc_score_asm(10 * score_mult, sugar_stat, tail_length);
        apple_x = get_random_num_asm(max_width);
        apple_y = get_random_num_asm(max_height);
        tail_length++;
    }

    // chance to spawn special abilities
    if (is_item_spawned == 0 && (get_random_num_asm(50) == 1)) {
        item_x = get_random_num_asm(max_width);
        item_y = get_random_num_asm(max_height);
        
        int r = get_random_num_asm(100);
        if (r < 45) item_symbol = '$';
        else if (r < 90) item_symbol = '%';
        else item_symbol = 'T'; 

        // randomizing lifespan of abilites 
        int random_time = get_random_num_asm(3); 
        if (random_time == 0) item_life = 30;      
        else if (random_time == 1) item_life = 50; 
        else item_life = 70;                       

        is_item_spawned = 1;
    }

    // ate special ability
    if (is_item_spawned == 1 && head_x == item_x && head_y == item_y) {
        is_item_spawned = 0; 
        
        if (item_symbol == 'T') {
            got_totem = 1;
            current_score += calc_score_asm(100 * score_mult, 0, tail_length); 
        }
        else {
            tail_length++; 
            if (item_symbol == '$') {
                current_score += calc_score_asm(50 * score_mult, 0, tail_length); 
                game_speed = 40; 
                fast_time_left = 50; 
            } 
            else if (item_symbol == '%') {
                current_score += calc_score_asm(75 * score_mult, 0, tail_length);
                drunk_time_left = 40; 
            }
        }
    }
}

int main() {
    // clear leaderboard on every run
    for(int i = 0; i < 5; i++) {
        strcpy(top_players[i].name, "---");
        top_players[i].score = 0;
    }

    while(1) {
        main_menu(); 
        newgame();
        
        while (!game_over) {
            grid();
            keyinputs();
            maingame();
            Sleep(game_speed); 
        }
        
        system("cls"); 
        save_score(player_name, current_score);
        printf("\n=========================================\n");
        printf("               GAME OVER                 \n");
        printf("=========================================\n");
        printf(" Player: %s\n", player_name);
        printf(" Final Score: %d\n", current_score);
        printf("=========================================\n");
        printf("\nSaved to leaderboard.\n");
        printf("Press any key to go back to menu...\n");
        _getch();
    }
    return 0;
}
