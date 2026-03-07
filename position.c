#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "position.h"

#define NO_SQ 64
#define WHITE 0
#define BLACK 1

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
} Position;

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
    for (int i=0; pieces[i]!='\0'; i++){
        char thischar = pieces[i];
        if (thischar=='/'){cur_sq-=15; continue;}
        else if (isdigit(thischar)){cur_sq+=(thischar-'0'); continue;}
        else {add_piece(cur_sq, position, thischar);}
        cur_sq++;
    }

    position->side_to_move = (side=='w');

    position->castling_rights = 0;
    if (strchr(castles,'K')) position->castling_rights |= 1<<0;
    if (strchr(castles,'Q')) position->castling_rights |= 1<<1;
    if (strchr(castles,'k')) position->castling_rights |= 1<<2;
    if (strchr(castles,'q')) position->castling_rights |= 1<<3;

    position->en_passant = (ep[0]=='-') ? NO_SQ : (ep[1]-'1')*8 + (ep[0]-'a');

    position->halfmove = (uint16_t) halfmove;
    position->fullmove = (uint16_t) fullmove;

    for (int w=0; w<6; w++){
        position->occupancies[0] |= position->bitboards[w];
    } for (int b=0; b<6; b++){
        position->occupancies[1] |= position->bitboards[b+6];
    }
    position->occupancies[2] = position->occupancies[0] | position->occupancies[1];
}

char* unload_position(Position* position){
    static char fen[100];
    memset(fen, '\0', sizeof(fen));
    char* idx = fen;

    const char piece_to_char[] = "PNBRQKpnbrqk";

    //Pieces
    int empty = 0;
    int cur_sq = 56;
    for (int i=0; i<64; i++){
        int cur_piece = piece_at(cur_sq, position);
        if (cur_piece==NO_SQ) {empty++;}
        else {
            if (empty==0) {
                *idx++ = piece_to_char[cur_piece];
            }
            else { //In the case that there have been empties
                *idx++ = '0' + empty;
                empty = 0;
            } 
        }
        cur_sq++;

        if ((i % 8) == 7){
            if (empty > 0){
                *idx++ = '0' + empty;
                empty = 0;
            }

            if (i != 63) *idx++ = '/';

            cur_sq -= 15; // go down one rank (15 not 16)
        }
    }

    //Side
    *idx++ = ' ';
    *idx++ = position->side_to_move ? 'w' : 'b';

    //To Castle or Not
    *idx++ = ' ';
    if (position->castling_rights == 0){
        *idx++ = '-';
    } else {
        if (position->castling_rights & 1) *idx++ = 'K';
        if (position->castling_rights & 2) *idx++ = 'Q';
        if (position->castling_rights & 4) *idx++ = 'k';
        if (position->castling_rights & 8) *idx++ = 'q';
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