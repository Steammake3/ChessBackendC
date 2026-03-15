#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "position.h"
#include "zobrist.h"


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
uint8_t EDGEDISTS[64][8];
const int8_t DIRECTIONS[8] = {-9,-7,7,9,-8,-1,1,8};

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
    bool en_passanting = (end_sq==pos->en_passant && (start_piece%6)==0); //This makes it DRY

    //Undo pushed
    undo->captured = captured_piece;
    undo->castle = pos->castling_rights;
    undo->ep = pos->en_passant;
    undo->halfmove = pos->halfmove;
    undo->fullmove = pos->fullmove;
    undo->hash = pos->zobrist;
    undo->flags = 0;

    //Move making :)

    pos->en_passant = NO_SQ;

    //Castling rights
    if (start_piece==WK) pos->castling_rights &= 0b0011;
    if (start_piece==BK) pos->castling_rights &= 0b1100;
    if (start_sq==56 || end_sq==56) pos->castling_rights &= ~1;
    if (start_sq==63 || end_sq==63) pos->castling_rights &= ~2;
    if (start_sq==0 || end_sq==0) pos->castling_rights &= ~4;
    if (start_sq==7 || end_sq==7) pos->castling_rights &= ~8;
    //Zob
    pos->zobrist ^= zh_castles[undo->castle] ^ zh_castles[pos->castling_rights];

    //Actual Castling (Rook side)
    if(start_piece%6==5 && ABS(start_sq-end_sq)==2){
        int rook_from = (end_sq>start_sq)? start_sq+3 : start_sq-4;
        int rook_to   = (end_sq>start_sq)? start_sq+1 : start_sq+2;
        pos->bitboards[(side==WHITE?WR:BR)] ^= (1ULL<<rook_from)|(1ULL<<rook_to);
        pos->occupancies[side] ^= (1ULL<<rook_from)|(1ULL<<rook_to);
        //Zob
        pos->zobrist ^= zh_pieces[side==WHITE?WR:BR][rook_from];
        pos->zobrist ^= zh_pieces[side==WHITE?WR:BR][rook_to];
        //For the Undo
        undo->flags = 2;
    }

    //True move in progress, crazy i know right?
    pos->bitboards[start_piece] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
    pos->occupancies[side] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
    //Zob
    pos->zobrist ^= zh_pieces[start_piece][start_sq];
    pos->zobrist ^= zh_pieces[start_piece][end_sq];

    //Normal capture, so not en passant
    if(captured_piece!=NO_SQ && !(en_passanting)){
        pos->bitboards[captured_piece] ^= 1ULL<<end_sq;
        pos->occupancies[side^1] ^= 1ULL<<end_sq;
        //Zob
        pos->zobrist ^= zh_pieces[captured_piece][end_sq];
    }

    //En passant
    if (en_passanting) {
        int cap_sq = side==BLACK ? end_sq+8 : end_sq-8;
        pos->bitboards[(side^1)*6+0] ^= (1ULL<<cap_sq);
        pos->occupancies[side^1] ^= (1ULL<<cap_sq);
        //Zob
        pos->zobrist ^= zh_pieces[(side^1)*6+0][cap_sq];
        //For the Undo flags
        undo->flags = 1;
    }

    //Promotion
    if (start_piece%6==0 && (end_sq>>3==0 || end_sq>>3==7)){
        pos->bitboards[start_piece] ^= (1ULL << end_sq);
        pos->bitboards[6*side+4-promo] ^= (1ULL << end_sq);
        //Zob
        pos->zobrist ^= zh_pieces[start_piece][end_sq];
        pos->zobrist ^= zh_pieces[6*side+4-promo][end_sq];
        //You know the drill by now
        undo->flags = 3;
    }

    //Double pawn pushes
    if (start_piece%6==0 && ABS(start_sq-end_sq)==16){
        pos->en_passant = (start_sq+end_sq)/2;
        //Zob later
    }

    //Half&Full Move
    if (start_piece%6==0 ||  captured_piece!=NO_SQ){
        pos->halfmove = 0;
    } else {pos->halfmove++;}
    if (side==BLACK) pos->fullmove++;

    //Side
    pos->side_to_move = !pos->side_to_move;
    //Zob
    pos->zobrist ^= zh_side;

    //Update final occupancies & EP File Zob
    pos->occupancies[BOTH] = pos->occupancies[WHITE] | pos->occupancies[BLACK];
    if (undo->ep != NO_SQ)
        pos->zobrist ^= zh_ep_file[undo->ep % 8];
    if (pos->en_passant != NO_SQ)
        pos->zobrist ^= zh_ep_file[pos->en_passant % 8];
}

void unmake_move(Position *pos, uint16_t move, Undo *undo){
    pos->side_to_move = !pos->side_to_move;

    int start_sq = MOVE_FROM(move);
    int end_sq = MOVE_TO(move);
    int promo = MOVE_FLAGS(move);
    int side = pos->side_to_move;
    //Btw all detection for special moves is done in make_move (to be DRY)
    int start_piece = piece_at(end_sq, pos); //Bcz move has been made
    int captured_piece = undo->captured; //Does not include EP

    //Quick undo restoration
    pos->en_passant = undo->ep;
    pos->castling_rights = undo->castle;
    pos->halfmove = undo->halfmove;
    pos->fullmove = undo->fullmove;
    pos->zobrist = undo->hash;

    //Generic move is always done (except for promo)
    if (undo->flags == 3) { //promo
        int pawn = (side==WHITE ? WP : BP);

        pos->bitboards[start_piece] ^= (1ULL<<end_sq); // remove promoted piece
        pos->bitboards[pawn] ^= (1ULL<<start_sq);      // restore pawn

        pos->occupancies[side] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
    } else {
        pos->bitboards[start_piece] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
        pos->occupancies[side] ^= (1ULL<<start_sq) | (1ULL<<end_sq);
    }

    switch (undo->flags) {
        case 1: {//En passant
            int cap_sq = side==BLACK ? end_sq+8 : end_sq-8;
            pos->bitboards[(side^1)*6+WP] ^= (1ULL<<cap_sq);
            pos->occupancies[side^1] ^= (1ULL<<cap_sq);
            break;}
        
        case 2: {//Castling
            int rook_from = (end_sq>start_sq)? start_sq+3 : start_sq-4;
            int rook_to   = (end_sq>start_sq)? start_sq+1 : start_sq+2;
            pos->bitboards[(side==WHITE?WR:BR)] ^= (1ULL<<rook_from)|(1ULL<<rook_to);
            pos->occupancies[side] ^= (1ULL<<rook_from)|(1ULL<<rook_to);
            break;}
        
        case 3: break; //Promotion previously handled
        
        default: break; //Silent move
    }

    if (captured_piece!=NO_SQ){ //In case of normal capture
        pos->bitboards[captured_piece] ^= 1ULL<<end_sq;
        pos->occupancies[side^1] ^= 1ULL<<end_sq;
    }

    pos->occupancies[BOTH] = pos->occupancies[WHITE] | pos->occupancies[BLACK];
}

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

void precompute_edgedists(){
    for (uint8_t r=0; r<8; r++){
        for (uint8_t f=0; f<8; f++){
            uint8_t sq = r*8+f;
            uint8_t n=7-r, s=r, e=7-f, w=f;
            uint8_t ne=MIN(n,e), nw=MIN(n,w), se=MIN(s,e), sw=MIN(s,w);
            //GOOOOOOOOOOOOOOOO!
            EDGEDISTS[sq][0] = sw;
            EDGEDISTS[sq][1] = se;
            EDGEDISTS[sq][2] = nw;
            EDGEDISTS[sq][3] = ne;
            EDGEDISTS[sq][4] = s;
            EDGEDISTS[sq][5] = w;
            EDGEDISTS[sq][6] = e;
            EDGEDISTS[sq][7] = n;
        }
    }
}