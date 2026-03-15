test_in = ["e2e4", "b8c6", "e4e5", "f7f5", "g1e2", "d7d5"];

def sq2idx(sq : str):
    return (ord(sq[0].lower())-ord("a"))+(int(sq[1])-1)*8

q = []

for test in test_in:
    #define MAKE_MOVE(f,t,fl)  (uint16_t) ((f) | ((t)<<6) | ((fl)<<12))
    out = sq2idx(test[:2]) | (sq2idx(test[2:4])<<6)
    if len(test)==5:
        r = "qrbn"
        out |= (r.find(test[4]))<<12
    q.append(hex(out))

print(f"\nuint16_t moves[{len(q)}] = {{{', '.join(q)}}};\n")