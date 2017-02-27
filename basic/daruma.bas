r = rgb(1.00, 0.20, 0.20)
w = rgb(1.00, 1.00, 1.00)
b = rgb(0.00, 0.00, 0.00)
y = rgb(0.93, 0.91, 0.54)
g = rgb(0.75, 0.75, 0.75)
gcls
gmode 1
width = screensize(0)
height = screensize(1)
dim p(20*28) = {
0, 0, 0, 0, 0, 0, 0, r, r, r, r, r, r, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, r, r, r, r, r, r, r, r, r, r, 0, 0, 0, 0, 0,
0, 0, 0, 0, r, r, r, r, r, r, r, r, r, r, r, r, 0, 0, 0, 0,
0, 0, 0, r, r, r, r, w, w, w, w, w, w, r, r, r, r, 0, 0, 0,
0, 0, r, r, b, b, b, b, b, w, w, b, b, b, b, b, r, r, 0, 0,
0, 0, r, r, b, w, g, g, w, w, w, w, g, g, w, b, r, r, 0, 0,
0, 0, r, r, b, g, b, b, g, w, w, g, b, b, g, b, r, r, 0, 0,
0, r, r, r, w, g, b, b, g, w, w, g, b, b, g, w, r, r, r, 0,
0, r, r, r, w, w, g, g, w, w, w, w, g, g, w, w, r, r, r, 0,
0, r, r, r, w, w, w, w, w, w, w, w, w, w, w, w, r, r, r, 0,
0, r, r, w, b, b, b, w, w, w, w, w, w, b, b, b, w, r, r, 0,
r, r, r, w, b, b, w, b, w, w, w, w, b, w, b, b, w, r, r, r,
r, r, r, r, w, w, w, w, b, b, b, b, w, w, w, w, r, r, r, r,
r, r, r, r, r, w, w, w, w, w, w, w, w, w, w, r, r, r, r, r,
r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r,
r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r,
r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r,
r, r, r, r, y, r, r, r, y, r, r, y, r, r, r, y, r, r, r, r,
r, r, r, r, y, r, r, y, r, r, r, r, y, r, r, y, r, r, r, r,
r, r, r, y, r, r, r, y, r, r, r, r, y, r, r, r, y, r, r, r,
r, r, r, y, r, r, r, y, r, r, r, r, y, r, r, r, y, r, r, r,
r, r, r, y, r, r, r, y, r, r, r, r, y, r, r, r, y, r, r, r,
0, r, r, r, y, r, r, y, r, r, r, r, y, r, r, y, r, r, r, 0,
0, r, r, r, y, r, r, r, y, r, r, y, r, r, r, y, r, r, r, 0,
0, 0, r, r, r, y, r, r, y, r, r, y, r, r, y, r, r, r, 0, 0,
0, 0, 0, r, r, y, r, r, y, r, r, y, r, r, y, r, r, 0, 0, 0,
0, 0, 0, 0, r, r, r, r, r, r, r, r, r, r, r, r, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, r, r, r, r, r, r, r, r, 0, 0, 0, 0, 0, 0
}
patdef 1, 20, 28, p
dim c1(12) = {0, 0, 2, 2, width - 40, 0, -1, 1, 0, height - 18, 2, -2}

proc proceed(n)
  c1(n) = c1(n) + c1(n + 2)
  c1(n + 1) = c1(n + 1) + c1(n + 3)
  if c1(n) < 0 then
    c1(n) = -c1(n)
    c1(n + 2) = -c1(n + 2)
  endif
  if c1(n + 1) < 0 then
    c1(n + 1) = -c1(n + 1)
    c1(n + 3) = -c1(n + 3)
  endif
  if c1(n) > width - 18 then
    c1(n + 2) = -c1(n + 2)
    c1(n) = c1(n) + c1(n + 2) * 2
  endif
  if c1(n + 1) > height - 18 then
    c1(n + 3) = -c1(n + 3)
    c1(n + 1) = c1(n + 1) + c1(n + 3) * 2
  endif
endproc

patdraw 1, c1(0), c1(1)
patdraw 1, c1(4), c1(5)
patdraw 1, c1(8), c1(9)
do while 1
  wait(40)
  paterase 1, c1(0), c1(1) 
  paterase 1, c1(4), c1(5)
  paterase 1, c1(8), c1(9)
  call proceed(0)
  call proceed(4)
  call proceed(8)
  patdraw 1, c1(0), c1(1)
  patdraw 1, c1(4), c1(5)
  patdraw 1, c1(8), c1(9)
  if inkey(1) <> 0 then break
loop
