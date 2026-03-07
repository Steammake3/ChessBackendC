#include <stdio.h>
#include <string.h>
#include "position.h"
#include "zobrist.h"

int fen_check();
int zobrist_check();

int main(){
    return zobrist_check();
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