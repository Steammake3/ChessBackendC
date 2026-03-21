#include <stdio.h>
#include <time.h>
#include <string.h>
#include "main.h"
#define MAX_HISTORY 1024

Position game;
const char *info = "----\nHello! Use the following commands -> \n"
    "\ta1a1 - makes a move from a1 to a1 (obviously other moves like e2e4 also work)\n"
    "\t/d - prints ASCII repr of board (WHITE, black)\n"
    "\tQ - quits game\n"
    "\t/undo - undoes previous move\n"
    "\t/log - prints a list of all moves played so far\n\n"; //TODO!!!!

int main(int argc, char *argv[]){
    if (argc!=3){
        printf("USAGE - BOT time_control{float} side_of_bot{w|b}"); return 1;
    }

    float control;
    bool get_first_move;
    Undo undid[MAX_HISTORY]; memset(undid, '\0', sizeof(undid));
    uint16_t moves[MAX_HISTORY]; memset(undid, '\0', sizeof(undid));
    uint16_t halfmove_ctr = 0;

    if (sscanf(argv[1], "%f", &control) != 1){ //Time control
        printf("Invalid time control\n");
        return 1;
    }
    
    if (argv[2][0] == 'w' || argv[2][0] == 'b'){ //Side for bot to play
        get_first_move = (argv[2][0] == 'w');
    } else {
        printf("Input 'w' or 'b', lower case only!\n");
        return 1;
    }

    printf("%s", info);
    init_consts();
    char taken[6];
    uint16_t chosenmove = 0;
    load_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &game);
    while (0xA34){
        scanf("%5s", taken);
        if (taken[0]=='Q') break;
        else if (taken[0]=='/'){
            if (taken[1]=='d'){ //Print current position
                printf("\n------------\n%s------------\n\n", ascii_repr(&game));
            } else if (taken[1]=='l'){ //Print log TODO

            } else if (taken[1]=='u'){ //Undo TODO

            }
        } else {
            make_move(&game, str2move(taken), &undid);
            chosenmove = best_move(control);
            printf("%s\n", move2str(chosenmove));
            make_move(&game, chosenmove, &undid);
        }
    }
    return 0;
}

uint16_t str2move(const char move[]){
    char from = move[0]-'a'+(move[1]-'1')*8;
    char to = move[2]-'a'+(move[3]-'1')*8;
    char promo = 0;
    if (move[4]!='\0'){
        if (move[4]=='r') promo=1;
        if (move[4]=='b') promo=2;
        if (move[4]=='n') promo=3;
    }
    return MAKE_MOVE(from, to, promo);
}

char* move2str(uint16_t move){
    int from = MOVE_FROM(move);
    int to   = MOVE_TO(move);
    int flag = MOVE_FLAGS(move);
    static char move_str[6] = "Chess";
    sprintf(move_str, "%c%c%c%c",
        'a' + (from % 8),
        '1' + (from / 8),
        'a' + (to % 8),
        '1' + (to / 8)
    );
    // Add promotion piece if any (skipping q for efficiency)
    if (flag == 1) move_str[4] = 'r';
    else if (flag == 2) move_str[4] = 'b';
    else if (flag == 3) move_str[4] = 'n';
    else move_str[4] = '\0';

    return move_str;
}

uint16_t best_move(float time_control){
    clock_t start_time = clock();
    uint16_t best_move = 0;

    while (0xA34){
        //Invoke search, TODO

        float elapsed_time = (float)(clock()-start_time) / CLOCKS_PER_SEC;
        if (elapsed_time >= time_control){
            break;
        }
    }
    return best_move;
}

void init_consts(){
    init_attack_tables(); precompute_chebyshev(); precompute_edgedists(); init_zobrist();
}