w = screensize(0)
h = screensize(1)
for i = 0 to 1000
  x = rnd() * w
  y = rnd() * h
  gcolor rgb(rnd(), rnd(), rnd())
  if i = 0 then pset x, y else lineto x, y
next
