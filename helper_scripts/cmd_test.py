import subprocess
import sys
import os
import threading

if len(sys.argv) < 2:
    print("Bro gimme a cmd to run")
    sys.exit(1)

bot_exe = sys.argv[1:]

if not os.path.exists(bot_exe[0]):
    print(f"Bro gimme a VALID file name: '{bot_exe[0]}'")
    sys.exit(1)

# Start process cleanly
proc = subprocess.Popen(
    bot_exe, 
    stdin=subprocess.PIPE, 
    stdout=subprocess.PIPE, 
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1
)

def listen_to_engine():
    """Continuously prints engine output as soon as it arrives."""
    try:
        # readline blocks natively until a line comes, using 0% CPU
        for line in iter(proc.stdout.readline, ''):
            print(f"{line}", end='', flush=True)
    except Exception:
        pass

# Fire up the listener immediately
threading.Thread(target=listen_to_engine, daemon=True).start()

print("--- Engine connected? ---")

while True:
    try:
        user_input = input("")
        proc.stdin.write(user_input + "\n")
        proc.stdin.flush()
        
    except (KeyboardInterrupt, EOFError):
        print("\nExiting proxy...")
        break

proc.stdin.write("Q") #Quit
proc.stdin.flush()

proc.terminate()
proc.wait()
print("OK")