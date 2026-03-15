#include <stdio.h>
#include <string.h>
#include "position.h"
#include "zobrist.h"
#include "movegen.h"

int fen_check();
int zobrist_check();
int move_making_check();
int perft_test();
int perft_divide_test();
uint64_t perft(Position *pos, int depth);
void perft_divide(Position *pos, int depth);

int main(){
    return perft_test();
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

int perft_test(){
    Position pos;
    load_position("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", &pos);
    init_zobrist(); init_attack_tables(); precompute_edgedists();// Undo u;
    //make_move(&pos, 0xac3, &u);

    printf("FEN : %s\n", unload_position(&pos));
    for (int i=1; i<6; i++){
        printf("Depth %d : %llu\n", i, perft(&pos, i));
    } return 0;
    return 0;
}

int perft_divide_test(){
    Position pos;
    load_position("r3k2r/Ppp2ppp/1b3nbN/nPPp4/BB2P3/q4N2/Pp1P2PP/R2Q1RK1 w kq d6 0 2", &pos);
    //load_position("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", &pos);
    //load_position("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NKPP/RNBQ3R b - - 0 8", &pos); //After e1f2
    //load_position("rnb2k1r/pp1Pbppp/1qp5/8/2B5/8/PPP1NKPP/RNBQ3R w - - 1 9", &pos); //After d8b6
    //load_position("rnbq1k1r/pp1Pbppp/2p5/8/2B5/P7/1PP1NnPP/RNBQK2R b KQ - 0 8", &pos); //After a2a3
    //load_position("rnb2k1r/pp1Pbppp/2p5/q7/2B5/P7/1PP1NnPP/RNBQK2R w KQ - 1 9", &pos); //After d8b6
    init_zobrist(); init_attack_tables(); precompute_edgedists();// Undo u;
    //make_move(&pos, 0xac3, &u);

    printf("FEN : %s\n", unload_position(&pos));
    perft_divide(&pos, 1);
    return 0;
}

uint64_t perft(Position *pos, int depth){
    MoveList mv; mv.count=0;
    if (depth==0) return 1ULL;
    Undo undid; uint64_t nodes = 0ULL;

    generate_moves(pos, &mv);
    for (int i=0; i<mv.count; i++){
        make_move(pos, mv.moves[i], &undid);
        nodes += perft(pos, depth-1);
        unmake_move(pos, mv.moves[i], &undid);
    }
    return nodes;
}

void perft_divide(Position *pos, int depth) {
    MoveList mv; 
    mv.count = 0;
    generate_moves(pos, &mv);

    uint64_t total = 0ULL;

    Undo undid;

    for (int i = 0; i < mv.count; i++) {
        make_move(pos, mv.moves[i], &undid);
        uint64_t nodes = perft(pos, depth - 1);
        unmake_move(pos, mv.moves[i], &undid);

        // Print move in algebraic notation
        int from = MOVE_FROM(mv.moves[i]);
        int to   = MOVE_TO(mv.moves[i]);
        int flag = MOVE_FLAGS(mv.moves[i]);
        char move_str[6];
        sprintf(move_str, "%c%c%c%c",
            'a' + (from % 8),
            '1' + (from / 8),
            'a' + (to % 8),
            '1' + (to / 8)
        );
        // Add promotion piece if any
        if ((piece_at(from, pos)%6==0) && (to/8==7 || to/8==0)){
            if (flag == 0) move_str[4] = 'q';
            else if (flag == 1) move_str[4] = 'r';
            else if (flag == 2) move_str[4] = 'b';
            else if (flag == 3) move_str[4] = 'n';
            else move_str[4] = '\0';
        }
        else move_str[4] = '\0';

        printf("%s: %llu\n", move_str, nodes);
        total += nodes;
    }

    printf("Total nodes at depth %d: %llu\n", depth, total);
}