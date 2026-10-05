# 起動ロゴ 招き猫 v3（128x48 枠, 描画は行 1〜42）。v3 を縦横同率 (41/46) で縮小し、行 1〜42 に収めた版（起動画面の情報行 y44- と重ならない）
from PIL import Image, ImageDraw
W,H=128,48
im=Image.new('1',(W,H),0); d=ImageDraw.Draw(im); L=1
S=41/46; CX=60
def X(x): return int(round(CX+(x-CX)*S))
def Y(y): return int(round(1+(y-1)*S))
def B(x0,y0,x1,y1): return (X(x0),Y(y0),X(x1),Y(y1))
def P(pts): return [(X(x),Y(y)) for x,y in pts]
d.rounded_rectangle(B(45,30,85,47),radius=8,outline=L,fill=0,width=2)          # body
d.rounded_rectangle(B(34,16,48,40),radius=5,outline=L,fill=0,width=2)          # arm
d.polygon(P([(48,18),(51,1),(62,9)]),outline=L,fill=0)                         # ears
d.polygon(P([(70,9),(81,1),(84,18)]),outline=L,fill=0)
d.line(B(49,17,51,3),L); d.line(B(83,17,81,3),L)
d.line(B(52,6,55,10),L); d.line(B(80,6,77,10),L)
d.ellipse(B(43,6,87,34),outline=L,fill=0,width=2)                              # head
d.rounded_rectangle(B(31,6,47,22),radius=6,outline=L,fill=0,width=2)           # raised paw
for x in (36,39,42): d.line(B(x,7,x,10),L)
d.ellipse(B(36,13,42,18),outline=L)
d.arc((X(51),Y(13)-1,X(61),Y(21)+2),195,345,L,width=2); d.arc((X(70),Y(13)-1,X(80),Y(21)+2),195,345,L,width=2)  # eyes（縮小で潰れないよう縦に余裕）
d.line(B(64,21,66,21),L); d.point((X(65),Y(22)),L)                             # nose
d.arc(B(59,21,65,28),20,160,L); d.arc(B(65,21,71,28),20,160,L)                 # mouth
for (x0,y0,x1,y1) in ((46,20,55,22),(45,24,55,24),(46,28,55,26)):              # whiskers
    d.line(B(x0,y0,x1,y1),L); d.line(B(130-x0,y0,130-x1,y1),L)
d.line(B(51,35,79,35),L); d.line(B(51,36,79,36),L)                             # collar
d.ellipse(B(61,35,69,43),outline=L,fill=0); d.line(B(62,39,68,39),L); d.point((X(65),Y(41)),L)  # bell
d.ellipse(B(71,36,89,47),outline=L,fill=0,width=2)                             # koban
for x in (77,80,83): d.line(B(x,39,x,44),L)
d.rounded_rectangle(B(68,38,76,45),radius=3,outline=L,fill=0)                  # paw on koban
# 右下の「25Ja」（手打ちの太字ゴシック 6x9, 字間 1）
G={
'2':[".####.","##..##","....##","...##.","..##..",".##...","##....","##....","######"],
'5':["######","##....","##....","#####.","....##","....##","....##","##..##",".####."],
'J':["..####","....##","....##","....##","....##","....##","##..##","##..##",".####."],
'a':["......","......","......",".####.","....##",".#####","##..##","##..##",".#####"],
}
def draw_text(im,x,y,t,gap=1):
    for ch in t:
        for r,row in enumerate(G[ch]):
            for c,v in enumerate(row):
                if v=='#': im.putpixel((x+c,y+r),1)
        x+=len(G[ch][0])+gap
def width(t,gap=1): return sum(len(G[c][0]) for c in t)+gap*(len(t)-1)

draw_text(im, W-width('25Ja')-1, 42-9+1, '25Ja')
# gen_boot_logo.py 用 PNG（黒=点灯）
import os
from PIL import ImageOps
ImageOps.invert(im.convert('L')).convert('1').save(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'boot_logo_25ja.png'))
# 生成: python3 App/tools/gen_boot_logo.py App/tools/boot_logo_25ja.png -o App/ui/boot_logo_bitmap.c --zoom 1 --inset 0
