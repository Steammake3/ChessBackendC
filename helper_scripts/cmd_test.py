import subprocess, sys, os

if len(sys.argv) != 4:
    print("Bro gimme a cmd to run")
    sys.exit()

bot_exe = sys.argv[1:]

if not os.path.exists(bot_exe[0]):
    print("Bro gimme a VALID file name")
    sys.exit()

proc = subprocess.Popen(bot_exe, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

while True:
    try:
        o, e = proc.communicate(input("> "))
        print(F"> {o}")
    except KeyboardInterrupt:
        break

print("OK")