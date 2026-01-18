#include "sos_cmdgames.h"
#include "sos_cmds.h"
#include "sos_lunarlander.h"
#include "sos_typeracer.h"
#include "sos_mirrorgame.h"
#include "sos_tictactoe.h"
#include "sos_spaceshooter.h"
#include "sos_memorygame.h"
#include "sos_lifesim.h"
#include "sos_2048.h"
#include "sos_minesweeper.h"
#include "sos_breakout.h"
#include "sos_pong.h"
#include "sos_snake.h"
#include "sos_tetris.h"
#include "sos_logiccircuit.h"

void cmd_logiccircuit(CommandArgs args){
    run_logiccircuit_game();
}

void cmd_lunarlander(CommandArgs args){
    lunar_lander_run();
    lunar_lander_cleanup();
}

void cmd_typeracer(CommandArgs args){
    typeracer_game_run();
    typeracer_game_cleanup();
}

void cmd_mirrorgame(CommandArgs args){
    mirror_game_run();
    mirror_game_cleanup();
}

void cmd_tictactoe(CommandArgs args){
    tictactoe_run();
    tictactoe_cleanup();
}

void cmd_spaceshooter(CommandArgs args){
    spaceshooter_game_run();
    spaceshooter_game_cleanup();
}

void cmd_memorygame(CommandArgs args){
    memory_game_run();
    memory_game_cleanup();
}

void cmd_lifesim(CommandArgs args){
    life_game_run();
    life_game_cleanup();
}

void cmd_2048(CommandArgs args){
    game_2048_game_run();
    game_2048_game_cleanup();
}

void cmd_minesweeper(CommandArgs args){
    minesweeper_game_run();
    minesweeper_game_cleanup();
}

void cmd_breakout(CommandArgs args){
    breakout_game_run();
    breakout_game_cleanup();
}

void cmd_pong(CommandArgs args){
    pong_game_run();
    pong_game_cleanup();
}

void cmd_tetris(CommandArgs args) {
    tetris_game_run();
    tetris_game_cleanup();
}

void cmd_snake(CommandArgs args) {
    snake_game_run();
    snake_game_cleanup();
}