from pyperclip import paste, copy

txt = paste()
copy("\n".join(sorted(txt.split("\n"))))
print(":)\n")