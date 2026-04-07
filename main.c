#include <stdio.h>
#include <time.h>
#include <string.h>
#include "main.h"
#include "search.h"

Position game;
const char *info = "----\nHello! Use the following commands -> \n"
    "\ta1a1 - makes a move from a1 to a1 (obviously other moves like e2e4 also work)\n"
    "\t/d - prints ASCII repr of board, add a second character to show FEN too (WHITE, black)\n"
    "\tQ - quits game\n"
    "\t/undo - undoes previous move\n"
    "\t/log - prints a list of all moves played so far\n"
    "\t/s {SQ} - gets piece[s] at SQ\n"
    "\t#{FEN} - loads a FEN\n\n";

int main(int argc, char *argv[]){
    if (argc!=3){
        printf("USAGE - BOT time_control{float} side_of_bot{w|b}"); return 1;
    }

    float control;
    bool get_first_move;
    Undo undid[MAX_HISTORY]; memset(undid, '\0', sizeof(undid));
    uint16_t moves[MAX_HISTORY]; memset(moves, '\0', sizeof(moves));
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
    char taken[128];
    uint16_t chosenmove = 0;
    uint16_t playermove = 0;
    load_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &game);
    
    tt_init(32); //FREE LATER

    if (get_first_move){
        chosenmove = id_best_move(control);
        printf("%s\n", move2str(chosenmove));
        make_move(&game, chosenmove, &undid[halfmove_ctr]);
        moves[halfmove_ctr] = chosenmove;
        halfmove_ctr++;
    }

    while (0xA34){
        fgets(taken, sizeof(taken), stdin); taken[strcspn(taken, "\n")] = '\0';
        if (taken[0]=='Q' || taken[0]=='q') break;

        else if (taken[0]=='#'){ //Load FEN
            memmove(taken, taken + 1, strlen(taken)); //Remove #
            load_position(taken, &game);
            printf("Loaded %.9s... successfully!\n", taken);
            memset(undid, '\0', sizeof(undid)); memset(moves, '\0', sizeof(moves)); halfmove_ctr=0;//Clean up
            if (game.side_to_move == !get_first_move){ //If bot to move bruh
                printf("Deciding upon move rn... \n");
                chosenmove = id_best_move(control);
                printf("%s\n", move2str(chosenmove));
                make_move(&game, chosenmove, &undid[halfmove_ctr]);
                moves[halfmove_ctr] = chosenmove;
                halfmove_ctr++;
            }
        }

        else if (taken[0]=='/'){
            if (taken[1]=='d'){ //Print current position
                if (taken[2]=='\0')
                    printf("\n------------\n%s------------\n\n", ascii_repr(&game));
                else
                    printf("\n------------\n%s%s\n------------\n\n", ascii_repr(&game), unload_position(&game));
            } else if (taken[1]=='l'){ //Print log
                for (int mv_ctr = 0; mv_ctr < halfmove_ctr; mv_ctr += 2){
                    printf("%s", move2str(moves[mv_ctr]));
                    if (mv_ctr + 1 < halfmove_ctr) {
                        printf(" %s", move2str(moves[mv_ctr+1]));
                    }
                    printf("\n");
                }
            } else if (taken[1]=='u'){ //Undo
                if (halfmove_ctr > 0) {
                    halfmove_ctr--;
                    printf("***Undid %s\n", move2str(moves[halfmove_ctr]));
                    unmake_move(&game, moves[halfmove_ctr], &undid[halfmove_ctr]);
                }
            } else if (taken[1]=='s'){
                char spoew[4] = {0}; sscanf(taken, "/s %3s", spoew);
                uint8_t sq = str2move(spoew);
                printf("According to occupancies - %d\n", piece_at(sq, &game));
                for(int i=0;i<12;i++) printf("Piece %d: 0x%llx\n", i, game.bitboards[i]);
                printf("White occ: 0x%llx\n", game.occupancies[WHITE]);
                printf("Black occ: 0x%llx\n", game.occupancies[BLACK]);
            }

        } else { //PlayBOT
            //Check for check/stalemate (not the cleanest but it works)
            chosenmove = id_best_move(-1.0);
            if (chosenmove==0){ //Stalemate
                printf("DRAW!!\n"); break;
            } else if (chosenmove==UINT16_MAX){ //Checkmate
                printf("CHECKMATE!! %s wins!\n", get_first_move ? "WHITE" : "black"); break;
            }
            //Do move
            playermove = str2move(taken);
            make_move(&game, playermove, &undid[halfmove_ctr]);
            moves[halfmove_ctr] = playermove;
            halfmove_ctr++;

            chosenmove = id_best_move(control);
            if (chosenmove==0){ //Stalemate
                printf("DRAW!!\n"); break;
            } else if (chosenmove==UINT16_MAX){ //Checkmate
                printf("CHECKMATE!! %s wins!\n", get_first_move ? "black" : "WHITE"); break;
            }
            printf("%s\n", move2str(chosenmove));
            make_move(&game, chosenmove, &undid[halfmove_ctr]);
            moves[halfmove_ctr] = chosenmove;
            halfmove_ctr++;
        }
    }
    tt_free();
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

uint16_t id_best_move(float time_control){
    clock_t start_time = clock();
    //Give bot ts
    bot_time_control = time_control<0.0 ? 3.14159f : time_control; start = start_time;
    uint16_t best_move = 0;
    uint8_t depth = 1;
    uint16_t best_given_move;

    while (0xA34){
        if (time_control<0 && depth==1) best_move = get_best_move(&game, depth);
        elapsed_time = (float)(clock()-start_time) / CLOCKS_PER_SEC;
        if (elapsed_time >= time_control || time_control<0){
            break;
        }
        //Invoke search
        best_given_move = get_best_move(&game, depth);
        if (best_given_move && (best_given_move!=TIMEOUT_MOVE)) best_move = best_given_move; //Don't store nullmoves
        depth++;
    }
    if (time_control>0) printf("At depth %i, taking %f seconds: ", depth, elapsed_time);
    return best_move;
}

void init_consts(){
    init_attack_tables();
    precompute_chebyshev();
    precompute_edgedists();
    init_zobrist();
    precompute_betweens();
    precompute_hq_masks();
}