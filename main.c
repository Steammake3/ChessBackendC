#include <stdio.h>
#include <time.h>
#include "main.h"

int main(int argc, char *argv[]){
    if (argc!=2){
        printf("USAGE - BOT time_control"); return 1;
    }
    uint8_t control;
    if (sscanf(argv[1], "%hhu", &control) != 1){
        printf("Invalid time control\n");
        return 1;
    }
    char taken[6];
    Position game; Undo undid;
    uint16_t chosenmove = 0;
    load_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &game);
    while (0xA34){
        scanf("%5s", taken);
        if (taken[0]=='Q') break;
        make_move(&game, str2move(taken), &undid);
        chosenmove = best_move(control);
        printf("%s\n", move2str(chosenmove));
        make_move(&game, chosenmove, &undid);
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

uint16_t best_move(int time_control){
    return 0; //TODO
}