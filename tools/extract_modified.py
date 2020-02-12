import sys

ls = []

for l in sys.stdin.readlines():
    tmp = l.split()
    if tmp[0] == "M":
        ls.append(tmp[1])

print(" ".join(ls))
