#include "types.h"
#include "eval.h"

#define FLIP(sq) sq^0b111000

#define pawnValue 100
#define knightValue 300
#define bishopValue 310
#define rookValue 500
#define queenValue 900

int values[6] = {pawnValue, knightValue, bishopValue, rookValue, queenValue, 0}; //King should take

const int8_t middle_psts[6][64] = {
    {0, 0, 0, 0, 0, 0, 0, 0, //Black Pawns
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0},
    {-50, -40, -30, -30, -30, -30, -40, -50, //Black Knights
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50},
    {-20, -10, -10, -10, -10, -10, -10, -20, //Black Bishops
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20},
    {0, 0, 0, 0, 0, 0, 0, 0, //Black Rooks
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 5, 5, 0, 0, 0},
    {-20, -10, -10, -5, -5, -10, -10, -20, //Black Queens
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20},
    {-30,-40,-40,-50,-50,-40,-40,-30, //Black King
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20}
};
const int8_t endgame_psts[2][64] = {
    {0, 0, 0, 0, 0, 0, 0, 0,//Pawn
    80, 80, 80, 80, 80, 80, 80, 80,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
    20, 20, 20, 20, 20, 20, 20, 20,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,
    0, 0, 0, 0, 0, 0, 0, 0},
    {-50,-40,-30,-20,-20,-30,-40,-50, //King
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50}
};


int material_eval(Position *pos){
    int pawns = (popcount(pos->bitboards[WP])-popcount(pos->bitboards[BP]))*pawnValue;
    int knights = (popcount(pos->bitboards[WN])-popcount(pos->bitboards[BN]))*knightValue;
    int bishops = (popcount(pos->bitboards[WB])-popcount(pos->bitboards[BB]))*bishopValue;
    int rooks = (popcount(pos->bitboards[WR])-popcount(pos->bitboards[BR]))*rookValue;
    int queens = (popcount(pos->bitboards[WQ])-popcount(pos->bitboards[BQ]))*queenValue;

    int perspective = pos->side_to_move ? -1 : 1;
    return (pawns+knights+bishops+rooks+queens) * perspective;
}

int pst_eval(Position *pos, uint8_t endgame_weight){
    bool side_rn = pos->side_to_move; int perspective;
    int retval = 0;
    for (uint8_t side = WHITE; side<=BLACK; side++){
        perspective = side ? -1 : 1;
        for (uint8_t i = WP; i<=WK; i++){
            uint64_t current_pieces = pos->bitboards[side ? i+6 : i];
            while (current_pieces)
            {
                uint8_t cur_sq = pop_lsb(&current_pieces);
                if (i==WP || i==WK){
                    retval += perspective * lerp(middle_psts[i][side==WHITE? FLIP(cur_sq) : cur_sq],
                        endgame_psts[i==WK ? 1 : 0][side==WHITE? FLIP(cur_sq) : cur_sq],
                        endgame_weight);
                }
                else retval += perspective * middle_psts[i][side==WHITE? FLIP(cur_sq) : cur_sq];
            }
        }
    }
    perspective = side_rn ? -1 : 1;
    return retval * perspective;
}

int init_evaluate(Position *pos){
    uint8_t eg_weight = phase_to_eg_weight(compute_phase(pos));
    return material_eval(pos)+ pst_eval(pos, eg_weight); //TODO: game phase rn
}

int evaluate(Position *pos){
    return init_evaluate(pos);
}