#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "position.h"
#include "zobrist.h"

#define NO_SQ 64
#define WHITE 0
#define BLACK 1
#define BOTH 2

#define MOVE_FROM(m)   ((m) & 0x3F)          // 0b111111
#define MOVE_TO(m)     (((m) >> 6) & 0x3F)   // next 6 bits
#define MOVE_FLAGS(m)  (((m) >> 12) & 0xF)   // top 4 bits

#define MAKE_MOVE(f,t,fl)  (uint16_t) ((f) | ((t)<<6) | ((fl)<<12))
#define ABS(a) ((a>0) ? a : -a)

/*
typedef enum {
    WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK
} Pieces;
 

 
typedef struct {
    uint64_t bitboards[12]; //White Pawn, Knight, Bishop ... Black Pawn... (PNBRQK)
    uint64_t occupancies[3]; //White, Black, Both
    uint8_t en_passant; //0-63 for actual EP, if en_passant>>6 then no en_passant
    uint16_t halfmove;
    uint16_t fullmove;
    bool side_to_move;
    uint8_t castling_rights;
    uint64_t zobrist;
} Position;
 */

uint8_t CHEBYSHEV[64][64];

void add_piece(int index, Position *position, char piece){
    //Adds a piece to `position` at `index` via FEN (so Q is White Queen) 
    switch (piece){
        case 'P': position->bitboards[WP] |= 1ULL << index; break;
        case 'N': position->bitboards[WN] |= 1ULL << index; break;
        case 'B': position->bitboards[WB] |= 1ULL << index; break;
        case 'R': position->bitboards[WR] |= 1ULL << index; break;
        case 'Q': position->bitboards[WQ] |= 1ULL << index; break;
        case 'K': position->bitboards[WK] |= 1ULL << index; break;
        //Black
        case 'p': position->bitboards[BP] |= 1ULL << index; break;
        case 'n': position->bitboards[BN] |= 1ULL << index; break;
        case 'b': position->bitboards[BB] |= 1ULL << index; break;
        case 'r': position->bitboards[BR] |= 1ULL << index; break;
        case 'q': position->bitboards[BQ] |= 1ULL << index; break;
        case 'k': position->bitboards[BK] |= 1ULL << index; break;
    }
}

int piece_at(int index, Position *position){
    if (index < 0 || index > 63) return NO_SQ;
    if (!((position->occupancies[2]>>index)&1)){return NO_SQ;}
    for (int i=0; i<12; i++){
        if ((position->bitboards[i]>>index)&1){return i;}
    }
    return NO_SQ;
}

void load_position(const char fen[], Position *position){
    memset(position, 0, sizeof(Position));
    char pieces[72], side, castles[5], ep[3];
    int halfmove, fullmove;
    sscanf(fen, "%71s %c %4s %2s %d %d", pieces, &side, castles, ep, &halfmove, &fullmove);

    int cur_sq = 56;
    for (int i = 0; pieces[i] != '\0'; i++) {
        char c = pieces[i];

        if (c == '/') {cur_sq -= 16;}
        else if (isdigit(c)) {cur_sq += c - '0';}
        else {
            add_piece(cur_sq, position, c);
            cur_sq++;
        }
    }

    position->side_to_move = (side=='b');

    position->castling_rights = 0;
    if (strchr(castles,'K')) position->castling_rights |= 1<<3;
    if (strchr(castles,'Q')) position->castling_rights |= 1<<2;
    if (strchr(castles,'k')) position->castling_rights |= 1<<1;
    if (strchr(castles,'q')) position->castling_rights |= 1<<0;

    position->en_passant = (ep[0]=='-') ? NO_SQ : (ep[1]-'1')*8 + (ep[0]-'a');

    position->halfmove = (uint16_t) halfmove;
    position->fullmove = (uint16_t) fullmove;

    for (int w=0; w<6; w++){
        position->occupancies[0] |= position->bitboards[w];
    } for (int b=0; b<6; b++){
        position->occupancies[1] |= position->bitboards[b+6];
    }
    position->occupancies[2] = position->occupancies[0] | position->occupancies[1];

    init_hash(position);
}

char* unload_position(Position* position){
    static char fen[100];
    memset(fen, '\0', sizeof(fen));
    char* idx = fen;

    const char piece_to_char[] = "PNBRQKpnbrqk";

    //Pieces
    int empty;
    for (int rank = 7; rank >= 0; rank--) {
        empty = 0;
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int piece = piece_at(sq, position);
            if (piece == NO_SQ) {
                empty++;
            } else {
                if (empty > 0) {
                    *idx++ = '0' + empty;
                    empty = 0;
                }
            *idx++ = piece_to_char[piece];
            }
        }

        if (empty > 0){*idx++ = '0' + empty;}
        if (rank > 0) {*idx++ = '/';}
    }

    //Side
    *idx++ = ' ';
    *idx++ = position->side_to_move ? 'b' : 'w';

    //To Castle or Not
    *idx++ = ' ';
    if (position->castling_rights == 0){
        *idx++ = '-';
    } else {
        if (position->castling_rights & 8) *idx++ = 'K';
        if (position->castling_rights & 4) *idx++ = 'Q';
        if (position->castling_rights & 2) *idx++ = 'k';
        if (position->castling_rights & 1) *idx++ = 'q';
    }

    //En Passant the doom of all programmers (Rio has no trouble tho)
    *idx++ = ' ';
    if (position->en_passant == NO_SQ){
        *idx++ = '-';
    } else {
        *idx++ = 'a' + (position->en_passant % 8);
        *idx++ = '1' + (position->en_passant / 8);
    }

    //So 'tis the moves bruh
    idx += sprintf(idx, " %d %d", position->halfmove, position->fullmove);

    return fen;
}

void init_hash(Position *pos){
    init_zobrist();

    uint64_t hash = 0ULL;
    for (int i=0; i<64; i++){
        int piece_type = piece_at(i, pos);
        if (piece_type==NO_SQ) {continue;}
        hash ^= zh_pieces[piece_type][i];
    }

    hash ^= zh_castles[pos->castling_rights];
    hash ^= (pos->en_passant)==NO_SQ ? 0 : zh_ep_file[(pos->en_passant)%8];

    hash ^= pos->side_to_move ? zh_side : 0;

    pos->zobrist = hash;
}

void make_move(Position *pos, uint16_t move, Undo *undo){
    //Set up for ease
    int start_sq = MOVE_FROM(move);
    int end_sq = MOVE_TO(move);
    int promo = MOVE_FLAGS(move);
    int side = pos->side_to_move;

    int start_piece = piece_at(start_sq, pos);
    int captured_piece = piece_at(end_sq, pos);

    //Undo pushed
    undo->captured = captured_piece;
    undo->castle = pos->castling_rights;
    undo->ep = pos->en_passant;
    undo->hash = pos->zobrist;

    //Move making
    switch (start_piece%6){

        case (WK): //King
            
            if ((start_sq==4 || start_sq==60)){ //Declining the right to castle
                pos->castling_rights &= (side==BLACK) ? 0b1100 : 0b0011;
            }

            if (ABS((int)start_sq-end_sq) == 2){ //Castling
                if (start_sq<end_sq){ //Kingside
                    if (side==BLACK){ //Black Kingside
                        pos->bitboards[BK] ^= (1ULL<<60) | (1ULL<<62);
                        pos->bitboards[BR] ^= (1ULL<<63) | (1ULL<<61);
                        pos->occupancies[BLACK] ^= (0xFULL << 60); //Combines the top ones tbh
                    } else { //White Kingside
                        pos->bitboards[WK] ^= (1ULL<<4) | (1ULL<<6);
                        pos->bitboards[WR] ^= (1ULL<<7) | (1ULL<<5);
                        pos->occupancies[WHITE] ^= 0xF0ULL; //Combines the top ones btw
                    }
                } else { //Queenside
                    if (side==BLACK){ //Black Queenside
                        pos->bitboards[BK] ^= (1ULL<<60) | (1ULL<<58);
                        pos->bitboards[BR] ^= (1ULL<<56) | (1ULL<<59);
                        pos->occupancies[BLACK] ^= (0b11101ULL << 56); //Combines the top ones tbh
                    } else { //White Queenside
                        pos->bitboards[WK] ^= (1ULL<<4) | (1ULL<<2);
                        pos->bitboards[WR] ^= (1ULL<<0) | (1ULL<<3);
                        pos->occupancies[WHITE] ^= 0b11101; //Combines the top ones (manually) btw
                    }
                }
            } else { //Regular move
                pos->bitboards[side==BLACK? BK : WK] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
                if (captured_piece!=NO_SQ) { //If a capture
                    pos->bitboards[captured_piece] ^= (1ULL<<end_sq);
                    pos->occupancies[side] ^= (1ULL<<start_sq);
                    pos->occupancies[side^1] ^= (1ULL<<end_sq);
                } else {
                    pos->occupancies[side] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
                }
            }

            if (end_sq==(side==BLACK ? 0 : 56) || end_sq==(side==BLACK ? 7 : 63)){
                pos->castling_rights &= ~(side==BLACK ? (end_sq==0 ? 4 : 8) : (end_sq==56 ? 1 : 2));
            }
            break;
        
        case (WR): //TODO: rook
    }
    pos->occupancies[BOTH] = pos->occupancies[WHITE] | pos->occupancies[BLACK];
}

void unmake_move(Position *pos, uint16_t move, Undo *undo){}

void precompute_chebyshev(){
    //ChatGPT generated, I don't care bcz it's just a formula
    for (int a = 0; a < 64; a++) {
        int file_a = a & 7;
        int rank_a = a >> 3;
        for (int b = 0; b < 64; b++) {
            int file_b = b & 7;
            int rank_b = b >> 3;

            int df = file_a - file_b;
            if (df < 0) df = -df;

            int dr = rank_a - rank_b;
            if (dr < 0) dr = -dr;

            CHEBYSHEV[a][b] = (df > dr) ? df : dr;
        }
    }
}

