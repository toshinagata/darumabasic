w=screensize(0)/2
h=screensize(1)/2
dim col(4)
col={rgb(1,0,0),rgb(0,1,0),rgb(0,0,1),rgb(1,1,0)}
p=0
b#=6.0
do
 redraw 0
 gcls
 for i=0 to 3
  gcolor col(i)
  for r=0 to w*2
   t#=b#*log(1+r*0.1)+3.1416*(i*0.5-p/20.0)
   x#=w+r*cos(t#)
   y#=h+r*sin(t#)
   if r=0 then
	pset x#,y#
   else
	lineto x#,y#
   endif
  next
 next
 p=(p+1)%40
 redraw 1
loop
