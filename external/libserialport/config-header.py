pattern = str.join('', open("config-pattern").readlines())
for line in open("config-fields").readlines():
    print pattern.format(*line.rstrip("\n").split(","))
