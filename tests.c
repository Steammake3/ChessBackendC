#include <stdio.h>
#include <string.h>
#include "position.h"
#include "zobrist.h"

int fen_check();
int zobrist_check();
int move_making_check();

int main(){
    return move_making_check();
}

int fen_check()
{
    Position position;
    char fen[101];
    for (int i = 0; i < 3; i++)
    {
        fgets(fen, 100, stdin);
        fen[strcspn(fen, "\n")] = 0;
        load_position(fen, &position);
        printf("%s\n", unload_position(&position));
    }
    return 0;
}

int zobrist_check(){
    init_zobrist();

    for (int piece=0; piece<12; piece++){
        for (int sq=0; sq<64; sq++){
            printf("%x\n", zh_pieces[piece][sq]);
        }
    }

    for (int castle=0; castle<4; castle++){printf("%x\n", zh_castles[castle]);}
    for (int file=0; file<8; file++){printf("%x\n", zh_ep_file[file]);}

    printf("%x\n", zh_side);
    return 0;
}

int move_making_check(){
    Position pos;
    Undo stack[1024];
    int stk_ptr = 0;

    uint16_t moves[6] = {0x70c, 0xab9, 0x91c, 0x975, 0x306, 0x8f3};

    load_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &pos);
    for (int i=0; i<6; i++){
        //printf("move %d: from %d to %d\n",i,MOVE_FROM(moves[i]),MOVE_TO(moves[i]));
        make_move(&pos, moves[i], &stack[stk_ptr++]);
    } for (int i=5; i>-1; i--){
        //printf("move %d: from %d to %d\n",i,MOVE_FROM(moves[i]),MOVE_TO(moves[i]));
        unmake_move(&pos, moves[i], &stack[--stk_ptr]);
    }

    printf("%s\n", unload_position(&pos));
    //printf("r1bqkbnr/ppp1p1pp/2n5/3pPp2/8/8/PPPPNPPP/RNBQKB1R w KQkq d6 0 4");
    return 0;
}