import subprocess, queue, threading, time, datetime
import chess, chess.pgn
import random

ENDED = "__SENTINEL_END__"

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
        self.outputs = ""
        
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
                self.outputs += line + "\n"

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

class Match:

    matches_played = 0
    season_id = "PLACEHOLDER"

    def __init__(self, botA : BotInstance, botB : BotInstance, maxMoves : int = 200):
        self.bot_a = botA #White
        self.bot_b = botB #Black

        if (self.bot_a.side_ == self.bot_b.side_):
            print("Bro black can't play black, try again")

        if (self.bot_a.side_ > self.bot_b.side_): #Bot B is somehow white
            self.bot_a, self.bot_b = self.bot_b, self.bot_a

        self.move_history = []
        self.game_running = False
        self.max_moves = maxMoves
        self._match_ran = False

    def play_match(self, createOutputPGN = True, add_output_to_PGN = True, file_template = "./match_logs/bot_match"):

        if self._match_ran:
            return

        self.bot_a.flush_initial_help_text()
        self.bot_b.flush_initial_help_text()

        shared_board = chess.Board()

        # Game node setup for the final PGN generation
        game_pgn = chess.pgn.Game()
        now = datetime.datetime.now()
        game_pgn.headers["Event"] = "Bot Battle Local Match"
        game_pgn.headers["Date"] = now.strftime("%Y.%m.%d")  # PGN standard format: YYYY.MM.DD
        game_pgn.headers["Time"] = now.strftime("%H:%M:%S")  # PGN standard format: HH:MM:SS
        game_pgn.headers["White"] = f"{self.bot_a.name}"
        game_pgn.headers["Black"] = f"{self.bot_b.name}"

        self.game_running = True

        print("\n--- Match Started! ---")
        self._match_ran = True # Even an unsuccesful run needs to be completely restarted from the ground up

        try:
            while self.game_running and len(self.move_history) < self.max_moves:
                white_move = self.bot_a.get_move()
                if (ENDED in white_move):
                    print(white_move[len(ENDED):])
                    break

                white_move = Match.process_and_patch_move(white_move, shared_board)
                shared_board.push(chess.Move.from_uci(white_move))

                self.move_history.append(white_move)
                print(f"{len(self.move_history) // 2 + 1}. {white_move} ", end="")

                self.bot_b.send(white_move)

                black_move = self.bot_b.get_move()
                if (ENDED in black_move):
                    print(black_move[len(ENDED):])
                    break

                black_move = Match.process_and_patch_move(black_move, shared_board)
                shared_board.push(chess.Move.from_uci(black_move))

                self.move_history.append(black_move)
                print(f"{black_move}")

                self.bot_a.send(black_move)

                time.sleep(0.5)

        except KeyboardInterrupt:
            print("\nMatch aborted by user.")
        except Exception as e:
            print(f"\nMatch errored out: {e}")

        print("\nGame Over. Cleaning up instances...")
        self.bot_a.terminate()
        self.bot_b.terminate()

        print(f"\nFinal Move Log: {self.move_history}")
        print("OK")

        game_pgn.add_line([chess.Move.from_uci(m) for m in self.move_history])

        output_filepath = f"{file_template}_Season{Match.season_id}_Match{Match.matches_played:4d}.pgn"

        matches_played += 1

        if createOutputPGN:
            with open(output_filepath, "w") as f:
                f.write(str(game_pgn))

        if createOutputPGN and add_output_to_PGN:
            with open(output_filepath, "a") as f:
                f.write(f"\n\nWhite\n{self.bot_a.outputs}\n\nBlack\n{self.bot_a.outputs}\n\n")
        if createOutputPGN:
            print(f"Archived PGN with metadata tags{" and outputs" if add_output_to_PGN else ""} successfully!")
                        

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

    def get_random_season_id():
        Match.season_id = hex(int(random.random() * 0xFF_FF_FF_FF)).upper()