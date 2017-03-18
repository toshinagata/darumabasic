'  SameGame for Daruma Basic
'  2016.2.13. Toshi Nagata
'  UTF-8 encoding

xx = screensize(0) / 16      '  盤面の幅
yy = screensize(1) / 16 - 2  '  盤面の高さ
n = 5     '  駒の種類
num = 0   '  残り駒の数
point = 0 '  ポイント
cx = 0    '  現在のカーソル位置 (x)
cy = 0    '  現在のカーソル位置 (y)

dim board(xx, yy)  '  盤面

proc disppiece(x, y)
  local c, col, bcol, s$
  c = board(x, y)
  if c = 1 then
    col = 1 : s$ = "♦"
  elseif c = 2 then
    col = 2 : s$ = "♥"
  elseif c = 3 then
    col = 3 : s$ = "♣"
  elseif c = 4 then
    col = 4 : s$ = "♠"
  elseif c = 5 then
    col = 5 : s$ = "＊"
  else
    col = 7 : s$ = "　"
  endif
  if x = cx and y = cy then
    bcol = 8
  else
    bcol = 0
  endif
  color col, bcol
  locate x * 2, y + 1
  print s$;
endproc

proc dispinfo()
  color 7, 0
  locate 0, 0 : print "【さめがめ】 ";
  print "残 "; num; " ";
  print "点 "; point; " ";
  clearline
endproc

proc display()
  local x, y
  for x = 0 to xx - 1
    for y = 0 to yy - 1
      call disppiece(x, y)
    next
  next
  call dispinfo()
endproc

func mark(x, y)
  local c, n
  c = board(x, y)
  board(x, y) = c + 100
  n = 1
  if x > 0 and board(x - 1, y) = c then n = n + mark(x - 1, y)
  if x < xx - 1 and board(x + 1, y) = c then n = n + mark(x + 1, y)
  if y > 0 and board(x, y - 1) = c then n = n + mark(x, y - 1)
  if y < yy - 1 and board(x, y + 1) = c then n = n + mark(x, y + 1)
  return n
endfunc

for x = 0 to xx - 1
  for y = 0 to yy - 1
    board(x, y) = int(rnd() * n) + 1
  next
next
num = xx * yy
cls
call display()
do
@redo
  locate cx * 2, cy + 1
  ch = inkey()
  if ch = 27 or ch = 113 or ch = 81 then break
  wx = cx
  wy = cy
  if ch = 28 then
    cx = mod(cx + 1, xx)
  elseif ch = 29 then
    cx = mod(cx - 1, xx)
  elseif ch = 30 then
    cy = mod(cy - 1, yy)
  elseif ch = 31 then
    cy = mod(cy + 1, yy)
  elseif ch = 13 then
    n = mark(cx, cy)
    if n = 1 then
      board(cx, cy) = board(cx, cy) - 100
    else
      for x = 0 to xx - 1
        for y = 0 to yy - 1
          if board(x, y) > 100 then call disppiece(x, y)
        next
      next
      for x = 0 to xx - 1
        y1 = yy - 1
        for y = yy - 1 to 0 step -1
          if board(x, y) < 100 then
            board(x, y1) = board(x, y)
            y1 = y1 - 1
          endif
        next
        do while y1 >= 0
          board(x, y1) = 0
          y1 = y1 - 1
        loop
      next
      x1 = 0
      for x = 0 to xx - 1
        if board(x, yy - 1) = 0 then @next
        if x <> x1 then
          for y = 0 to yy - 1
            board(x1, y) = board(x, y)
          next
        endif
        x1 = x1 + 1
        @next
      next
      for x1 = x1 to xx - 1
        for y = 0 to yy - 1
          board(x1, y) = 0
        next
      next
      point = point + (n - 2) * (n - 2)
      num = num - n
      call display()
    endif
    goto @redo
  endif
  if wx <> cx or wy <> cy then
    call disppiece(wx, wy)
    call disppiece(cx, cy)
  endif
loop
locate 0, yy + 1
color 7, 0
