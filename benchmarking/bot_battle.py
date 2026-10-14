import subprocess
import sys
import os
import threading
import queue
import time, datetime
import chess, chess.pgn
import match_emcee as mc

mc.Match.get_random_season_id()

if len(sys.argv) != 6:
    print("Usage: python bot_battle.py <Engine White cmd (no color)> <Engine Black cmd (no color)> <#matches to run>")
    sys.exit(1)

# Sample
# bot_battle.py bot.exe 1 bot.exe 3 50

global_outs = ["",""]

WHITE_PATH = sys.argv[1:3]
BLACK_PATH = sys.argv[3:5]

MATCH_COUNT = int(sys.argv[5])

if not os.path.exists(WHITE_PATH[0]):
    print(f"Error: Exe for white '{WHITE_PATH[0]}' not found.")
    sys.exit(1)

if not os.path.exists(BLACK_PATH[0]):
    print(f"Error: Exe for white '{BLACK_PATH[0]}' not found.")
    sys.exit(1)

print("Aight here we go bois (did u remember to disable output tho? Happy log surfing >:) )")

for _ in range(MATCH_COUNT):
    W_BOT = mc.BotInstance("Dhairya's Engine (White)", WHITE_PATH + ["w"])
    time.sleep(0.02) # For randomness
    B_BOT = mc.BotInstance("Dhairya's Engine (Black)", BLACK_PATH + ["b"])

    match = mc.Match(W_BOT, B_BOT)
    match.play_match()