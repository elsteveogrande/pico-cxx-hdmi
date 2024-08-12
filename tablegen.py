def popcount(d):
	assert len(d) in (8, 10)
	return sum(d)

def df(x):
	b = bin(int(round(x)))[2:]
	b = [int(bit) for bit in b]
	while len(b) < 8:  b = [0] + b
	b = list(reversed(b))
	return b

def dfr(d):
	x = 0
	add = 1
	d = d[:]
	while d:
		x += add if d[0] else 0
		d = d[1:]
		add <<= 1

	return x

def encode(x, neighbors=False):
	d = df(x)
	q = [None] * 10

	q[9] = 0
	q[8] = 1
	q[0] = d[0]
	for i in range(1, 8):  q[i] = q[i - 1] ^ d[i]

	if neighbors:
		pq = abs(5 - popcount(q))
		qm1 = encode(max(0, x - 1))
		pqm1 = abs(5 - popcount(qm1))
		qp1 = encode(min(255, x + 1))
		pqp1 = abs(5 - popcount(qp1))
		m = min(pq, pqm1, pqp1)
		if pqm1 == min(pq, pqm1, pqp1):
			return qm1
		if pqp1 == min(pq, pqm1, pqp1):
			return qp1

	return q

def decode(d):
	assert len(d) == 10
	assert d[8]
	assert not d[9]
	q = [None] * 8
	d = d[:]
	q[0] = d[0]
	for i in range(1, 8):  q[i] = d[i] ^ d[i - 1]
	return dfr(q)

m = 1 << 5
d = 256 / m
x = d / 2
levels = []

while True:
	xr = round(x)
	if xr > 255: break
	enc = encode(xr, neighbors=True)
	assert decode(enc) in (xr - 1, xr, xr + 1)
	levels.append(enc)
	c = ("0b" + "".join(reversed(list(str(bit) for bit in enc))))
	print("%-12s,  /* %3d */" % (c, int(x)))
	x += d

# table = []

# for r in range(8):
# 	for g in range(8):
# 		for b in range(8):
# 			table.append((levels[b], levels[g], levels[r]))

# # for x in table:
# # 	print(x)

# c0 = 0b1101010100
# c1 = 0b0010101011
# c2 = 0b0101010100
# c3 = 0b1010101011

# for c in (c0, c1, c2, c3):
# 	bs = [2, 2, 2, 2, 2, 1, 1, 1, 1, 1]
# 	ch2, ch1, ch0 = c0, c0, c
# 	for i in range(10):
# 		bs[i] |= ((ch2 & 1) ^ 1) << 7
# 		bs[i] |= ((ch2 & 1) ^ 0) << 6
# 		bs[i] |= ((ch1 & 1) ^ 1) << 5
# 		bs[i] |= ((ch1 & 1) ^ 0) << 4
# 		bs[i] |= ((ch0 & 1) ^ 1) << 3
# 		bs[i] |= ((ch0 & 1) ^ 0) << 2
# 		ch2 >>= 1
# 		ch1 >>= 1
# 		ch0 >>= 1
# 	print(["%02x" % b for b in bs])

# index = (2 << 6) + (7 << 3) + 2
# b, g, r = table[index]
# assert len(r) == 10
# assert len(g) == 10
# assert len(b) == 10
# bs = [2, 2, 2, 2, 2, 1, 1, 1, 1, 1]
# for i in range(10):
# 	print(r,g,b)
# 	bs[i] |= ((r[0]) ^ 1) << 7
# 	bs[i] |= ((r[0]) ^ 0) << 6
# 	bs[i] |= ((g[0]) ^ 1) << 5
# 	bs[i] |= ((g[0]) ^ 0) << 4
# 	bs[i] |= ((b[0]) ^ 1) << 3
# 	bs[i] |= ((b[0]) ^ 0) << 2
# 	r = r[1:]
# 	g = g[1:]
# 	b = b[1:]
# print(", ".join(["0x%02x" % b for b in bs]))


