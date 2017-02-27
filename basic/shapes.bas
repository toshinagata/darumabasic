@redo
print "Input number (1-6): ";
do
  c = inkey()
  if c = 3 or c = 27 then exit
loop until c > 48 and c <= 54
n = c - 48
print n
gcls
width = screensize(0)
height = screensize(1)
dim col#(4)
for i = 1 to 100
  w = rnd() * width * 0.5
  h = rnd() * height * 0.5
  x = rnd() * (width - w)
  y = rnd() * (height - h)
  col# = {rnd(), rnd(), rnd(), rnd() * 0.5 + 0.5}
  col#(rnd() * 3) = 1.0
  gcolor rgb(col#(0), col#(1), col#(2), col#(3))
  col# = {rnd(), rnd(), rnd(), rnd() * 0.5 + 0.5}
  col#(rnd() * 3) = 1.0
  fillcolor rgb(col#(0), col#(1), col#(2), col#(3))
  if n = 1 then
    line x, y, x + w, y + h
  elseif n = 2 then
    box x, y, w, h
  elseif n = 3 then
    rbox x, y, w, h, rnd() * 10 + 5, rnd() * 10 + 5
  elseif n = 4 then
    circle x + w / 2, y + h / 2, w / 2, h / 2
  elseif n = 5 then
    arc x + w / 2, y + h / 2, rnd() * 360, rnd() * 360, w / 2, h / 2
  elseif n = 6 then
    fan x + w / 2, y + h / 2, rnd() * 360, rnd() * 360, w / 2, h / 2
  endif
next
goto @redo
