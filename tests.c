#include <stdio.h>
#include <string.h>
#include "position.h"

int main(){
    Position position;
    char fen[101];
    for (int i=0; i<3; i++){
        fgets(fen, 100, stdin);
        fen[strcspn(fen, "\n")] = 0;
        load_position(fen, &position);
        printf("%s\n", unload_position(&position));
    }
    return 0;
}