
a = []
m = 8


for w in range(0, m):
	for z in range(0, m):
		for y in range(0, m):
			for x in range(0, m):
				a.append("x%dy%dz%dw%d" % (x, y, z, w))
print(a)
def access(x, y, z, w):
	x = x
	y = y * m
	z = z * m * m
	w = w * m * m * m
	return a[x + y + z + w]

print(access(1, 0, 1, 0))
print(access(2, 2, 2, 4))