import subprocess
import sys
import os
import threading
import queue
import time, datetime
import chess, chess.pgn

ENDED = "__SENTINEL_END__"

if len(sys.argv) != 5:
    print("Usage: python bot_battle.py <Engine White cmd (no color)> <Engine Black cmd (no color)>")
    sys.exit(1)

# Sample
# bot_battle.py bot.exe 1 bot.exe 3

global_outs = ["",""]

WHITE_PATH = sys.argv[1:3]
BLACK_PATH = sys.argv[3:]

if not os.path.exists(WHITE_PATH[0]):
    print(f"Error: Exe for white '{WHITE_PATH[0]}' not found.")
    sys.exit(1)

if not os.path.exists(BLACK_PATH[0]):
    print(f"Error: Exe for white '{BLACK_PATH[0]}' not found.")
    sys.exit(1)

class BotInstance:
    """Manages an isolated engine process and its own output queue."""
    def __init__(self, name, exe_path):
        self.name = name
        self.side_ = 0 if exe_path[-1] == "w" else 1
        self.expected_time = int(exe_path[1])
        self.proc = subprocess.Popen(
            exe_path,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        self.output_queue = queue.Queue()
        
        # Isolated thread to capture this specific bot's stdout
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()

    def _reader(self):
        try:
            for line in iter(self.proc.stdout.readline, ''):
                if line:
                    self.output_queue.put(line.strip())
        except Exception:
            pass

    def send(self, cmd):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def get_move(self, timeout=30.0):
        """Blocks until the bot outputs its evaluation line, then parses the move."""
        start_time = time.time()
        while True:
            if (time.time() - start_time) > max(timeout, self.expected_time+3): # 3 second leeway
                raise TimeoutError(f"{self.name} took too long to move.")
            try:
                line = self.output_queue.get_nowait()
                global_outs[self.side_] += line + "\n"

                if "!" in line: # SOme sort of end condition
                    return ENDED + line
                # Look for your engine's specific output format
                elif "taking" in line and "seconds:" in line:
                    parts = line.split(":")
                    if len(parts) >= 2:
                        return parts[-1].strip()
                        
            except queue.Empty:
                time.sleep(0.01)

    def flush_initial_help_text(self, lines_to_discard=10):
        """Blocks briefly to consume and discard the startup help menu."""
        # Give the engine process a tiny fraction of a second to print its help menu
        time.sleep(0.1)
        for _ in range(lines_to_discard):
            try:
                # Discard the line
                self.output_queue.get_nowait()
            except queue.Empty:
                break

    def terminate(self):
        try:
            self.send("Q")
        except:
            pass
        self.proc.terminate()

print("Aight here we go bois (did u remember to disable output tho? Happy log surfing >:) )")
W_BOT = BotInstance("White Bot", WHITE_PATH + ["w"])
time.sleep(0.02) # For randomness
B_BOT = BotInstance("Black Bot", BLACK_PATH + ["b"])

W_BOT.flush_initial_help_text()
B_BOT.flush_initial_help_text()


shared_board = chess.Board()

# Game node setup for the final PGN generation
game_pgn = chess.pgn.Game()
now = datetime.datetime.now()
game_pgn.headers["Event"] = "Bot Battle Local Match"
game_pgn.headers["Date"] = now.strftime("%Y.%m.%d")  # PGN standard format: YYYY.MM.DD
game_pgn.headers["Time"] = now.strftime("%H:%M:%S")  # PGN standard format: HH:MM:SS
game_pgn.headers["White"] = "My Engine (White)"
game_pgn.headers["Black"] = "My Engine (Black)"

move_history = []
max_moves = 200  

def process_and_patch_move(raw_move_str, board):
    """
    Uses the official board state to accurately verify and patch promotions.
    """
    if len(raw_move_str) == 4:
        # Convert square names (like 'e7') to python-chess square indices
        from_square = chess.parse_square(raw_move_str[:2])
        to_square = chess.parse_square(raw_move_str[2:])
        
        # Check if the moving piece is a Pawn
        piece = board.piece_at(from_square)
        if piece and piece.piece_type == chess.PAWN:
            # Check if it is landing on the promotion rank (Rank 8 for White, Rank 1 for Black)
            target_rank = chess.square_rank(to_square)
            if target_rank == 7 or target_rank == 0:  # 0-indexed ranks (7 is 8th rank, 0 is 1st rank)
                return raw_move_str + 'q'
                
    return raw_move_str

move_history = []
game_running = True
max_moves = 200  # Safety limit to prevent infinite loops during testing

print("\n--- Match Started! ---")

try:
    while game_running and len(move_history) < max_moves:
        white_move = W_BOT.get_move()
        if (ENDED in white_move):
            print(white_move[len(ENDED):])
            break

        white_move = process_and_patch_move(white_move, shared_board)
        shared_board.push(chess.Move.from_uci(white_move))

        move_history.append(white_move)
        print(f"{len(move_history) // 2 + 1}. {white_move} ", end="")

        B_BOT.send(white_move)

        black_move = B_BOT.get_move()
        if (ENDED in black_move):
            print(black_move[len(ENDED):])
            break

        black_move = process_and_patch_move(black_move, shared_board)
        shared_board.push(chess.Move.from_uci(black_move))

        move_history.append(black_move)
        print(f"{black_move}")

        W_BOT.send(black_move)

        time.sleep(0.5)

except KeyboardInterrupt:
    print("\nMatch aborted by user.")
except Exception as e:
    print(f"\nMatch errored out: {e}")

print("\nGame Over. Cleaning up instances...")
W_BOT.terminate()
B_BOT.terminate()

print(f"\nFinal Move Log: {move_history}")
print("OK")

game_pgn.add_line([chess.Move.from_uci(m) for m in move_history])

# 3. Write out to your archival file
with open("bot_match.pgn", "w") as f:
    f.write(str(game_pgn))

with open("bot_match.pgn", "a") as f:
    f.write(f"\n\nWhite\n{global_outs[0]}\n\nBlack\n{global_outs[1]}\n\n")

print("Archived PGN with metadata tags successfully!")