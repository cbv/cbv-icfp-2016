structure Shape:SHAPE =
struct

type point = int * int (* in Cartesian x-y coordinate *)

datatype shape =
    Rect of point * point (* bottom-left and upper-right *)
  | Disc of point * int (* center and radius *)
  | Intersection of shape * shape
  | Union of shape * shape
  | Without of shape * shape
  | Translate of shape * (int * int)
  | ScaleDown of shape * (int * int) (* x factor, y factor *)
  | ScaleUp of shape * (int * int) (* x factor, y factor *)
  | Line of point * point
(*  | Rotate of shape * int *) (* degrees + is counterclockwise*)

fun square (x : int) : real = real x * real x
fun deg2rad (deg:int) = (real deg) * (Math.pi/180.0)
fun min4 (x:int) (y:int) (z:int) (w:int) =
    Int.min (Int.min(x,y),Int.min(z,w))

(* Purpose: contains(s,p) == true if p is in the shape,
   or false otherwise *)
fun contains (s : shape, p as (x,y) : point) : bool =
    case s of
        Rect ((xmin,ymin),(xmax,ymax)) =>
            xmin <= x andalso x <= xmax andalso
            ymin <= y andalso y <= ymax
      | Disc ((cx,cy),r) =>
            square(x - cx) + square(y - cy) <= square r
      | Intersection(s1,s2) => contains(s1,p) andalso contains(s2,p)
      | Union(s1,s2) => contains(s1,p) orelse contains(s2,p)
      | Without (s1,s2) => contains (s1,p) andalso (not (contains (s2,p)))
      | Translate (s,(tx,ty)) => contains(s , (x - tx, y - ty))
      | ScaleDown (s,(xf,yf)) => contains (s , (xf * x, yf * y))
      | ScaleUp (s,(xf,yf)) => contains (s , (x div xf, y div yf))
      | Line ((x1, y1), (x2, y2)) =>
        let val (x1, y1) = (Real.fromInt x1, Real.fromInt y1)
            val (x2, y2) = (Real.fromInt x2, Real.fromInt y2)
            val (x, y) = (Real.fromInt x, Real.fromInt y)
            val d = Real./ (Real.abs (
(y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1),
                      Math.sqrt ((y2 - y1) * (y2 - y1) +
                                 (x2 - x1) * (x2 - x1)))
        in
            d < 0.5 andalso
            ((x >= x1 andalso x <= x2) orelse (x >= x2 andalso x <= x1)) andalso
            ((y >= y1 andalso y <= y2) orelse (y >= y2 andalso y <= y1))
        end
(*      | Rotate (s,deg) => contains(s, (round(x * Math.cos(deg2rad(-1*deg))),
                                       round(y * Math.sin(deg2rad(-1*deg)))))
*)

(* boundingbox s computs a rectangle r (lower-left,upper-right),
   such that if p is in s then p is in r
   *)
fun boundingbox1 (s : shape) : point * point =
    case s of
        Rect b => b
      | Without (s1,s2) => boundingbox1 s1 (* conservative *)
      | Translate (s,(x,y)) =>
            let val ((minx,miny),(maxx,maxy)) = boundingbox1 s in
                ((minx + x,miny + y),
                 (maxx + x,maxy + y))
            end
      | Disc((cx,cy),r) => ((cx - r, cy - r),
                            (cx + r+1, cy + r+1))
      | Union (s1,s2) =>
            let val ((s1minx,s1miny), (s1maxx,s1maxy)) = boundingbox1 s1
                val ((s2minx,s2miny), (s2maxx,s2maxy)) = boundingbox1 s2
            in
                ((Int.min(s1minx,s2minx),
                  Int.min(s1miny,s2miny)),
                 (Int.max(s1maxx,s2maxx),
                  Int.max(s1maxy,s2maxy)))
            end
      | Intersection (s1,s2) =>
            let val ((s1minx,s1miny), (s1maxx,s1maxy)) = boundingbox1 s1
                val ((s2minx,s2miny), (s2maxx,s2maxy)) = boundingbox1 s2
            in
               if s1minx > s2maxx orelse s2minx > s1maxx orelse
                  s1miny > s2maxy orelse s2miny > s1maxy
               then ((s1minx,s1miny),(s1minx,s1miny))
               else
                ((Int.max(s1minx,s2minx),
                  Int.max(s1miny,s2miny)),
                 (Int.min(s1maxx,s2maxx),
                  Int.min(s1maxy,s2maxy)))
            end
      | ScaleDown(s,(xf,yf)) =>
            let val ((xmin,ymin),(xmax,ymax)) = boundingbox1 s
            in
                ((xmin div xf, ymin div yf),
                 (xmax div xf, ymax div yf))
            end
      | ScaleUp(s,(xf,yf)) =>
            let val ((xmin,ymin),(xmax,ymax)) = boundingbox1 s
            in
                ((xmin * xf, ymin * yf),
                 ((xmax) * xf, (ymax) * yf))
            end
      | Line b => b



fun boundingbox(s:shape): point*point =
    let val ((minx,miny),(maxx,maxy)) = boundingbox1(s) in
        ((minx,miny),(maxx,maxy))
    end

fun size (s : shape) :int* int =
    let
      val ((minx,miny),(maxx,maxy)) = boundingbox s
    in
      (maxx-minx, maxy-miny)
    end

(* rectangle border with r as the inside border *)
fun rectb (r as ((minx,miny) : point, (maxx,maxy) : point),
                thickness : int) =
    Without (Rect ((minx - thickness, miny - thickness),
                   (maxx + thickness, maxy + thickness)),
             Rect r)


(* use
writeshape_bb (s : shape, filename : string) : unit
to write a shape s to file <filename>
*)

(* ---------------------------------------------------------------------- *)
(* BMP writing code *)
local
    (* http://www.soc.napier.ac.uk/~cs66/course-notes/sml/writebmp.txt *)

    fun getInt(s,i) = Word8.toInt(Word8Vector.sub(s,i));
    fun get2(s,i)=getInt(s,i)+256*getInt(s,i+1);
    fun get4(s,i)=get2(s,i)+256*256*get2(s,i+2);
    fun ttt 0 = 1|ttt 1= 2|ttt 2=  4|ttt 3=8|ttt 4=16|ttt 5=32|ttt 6=64|ttt 7=128|ttt 8=256|ttt _ = raise Fail "unimplemented"
    fun bits(b,s,n)=((b div ttt(s*n)) mod (ttt n));
    fun pad4 x = ~4*(~x div 4)

    fun readbmp s = let
                        val input = BinIO.inputN
                        val fh = BinIO.openIn s;
                        val header = input(fh,54)
                        val bitspp = get2(header,28)
                        val colTab = input(fh,get4(header,10) - 54)
                        val bitMap = input(fh,get4(header,34))
                        val w = get4(header,18)
                        val h = get4(header,22)
                        val dy = pad4((bitspp * w) div 8)
                        fun outrange(x,y)=x<0 orelse y<0 orelse x>=w orelse y>=h
                        val r = 8 div bitspp
                        fun col(x,y) = if outrange(x,y) then 0 else
                            bits(getInt(bitMap,dy*y+(x div r)),r-1-(x mod r),bitspp)
                    in (w,h,colTab,col)
                    end

    fun mk2 v = Word8Vector.fromList[Word8.fromInt(v mod 256), Word8.fromInt(v div 256 mod 256)];
    fun mk4 v = Word8Vector.concat[mk2 (v mod 65536),mk2 (v div 65536)];
    fun upto n m = if n=m then [n] else n::(upto (n+1) m);
    fun header w h c = let
                           val size = Word8Vector.length in
                               Word8Vector.concat[
                                                  Word8Vector.fromList(map(Word8.fromInt o ord) [#"B",#"M"]),
                                                  mk4((pad4 w)*h+size c+54),
                                                  mk4 0, mk4 (size c+54), mk4 40,
                                                  mk4 w,	mk4 h,	mk2 1,	mk2 8,	mk4 0,	mk4 ((pad4 w)*h),
                                                  mk4 0,	mk4 0,	mk4 256,	mk4 256] end;

    (* width, height, colour table, (x,y)->pixel value, filename *)
    fun writebmp (w,h,c,t) s = let
                                   val fh = BinIO.openOut s
                                   val waste=BinIO.output(fh,header w h c)
                                   val waste'=BinIO.output(fh,c)
                                   val padw = pad4 w
                                   fun f i = Word8.fromInt(t(i mod padw, i div padw))
                                   val waste'' = Word8Vector.tabulate(padw*h,f)
                               in BinIO.output(fh,waste'');BinIO.closeOut fh; s
                               end

        (* FIXME: based on my reading of the bitmap format, I think there should be a way to just
           have two entries, but the following doesn't display:
           Word8Vector.fromList [0wx0,0wx0,0wx0,0wx0,0wxFF,0wxFF,0wxFF,0wx00];
           so I grabbed a colormap with both black and white from some bmp *)
    val colormap =
        Word8Vector.fromList
  [0wx0,0wx0,0wx0,0wx0,0wxFF,0wxFF,0wxFF,0wx0,0wx82,0wx82,0wxFE,0wx0,0wxC8,
   0wxC8,0wxFF,0wx0,0wx45,0wx45,0wx86,0wx0,0wxC5,0wxCA,0wx7A,0wx0,0wxE6,0wxE8,
   0wxC5,0wx0,0wx68,0wx6B,0wx40,0wx0,0wxD5,0wxA0,0wx78,0wx0,0wxE1,0wxE1,0wxE1,
   0wx0,0wxE2,0wxD5,0wxC6,0wx0,0wxB5,0wx99,0wx8B,0wx0,0wxB2,0wxB2,0wxB2,0wx0,
   0wxC0,0wxC0,0wxC0,0wx0,0wx65,0wx65,0wx65,0wx0,0wx97,0wx97,0wx97,0wx0,0wxEE,
   0wxEE,0wxEE,0wx0,0wxCC,0wxCC,0wxCC,0wx0,0wx99,0wx0,0wx33,0wx0,0wxAA,0wxAA,
   0wxAA,0wx0,0wx88,0wx88,0wx88,0wx0,0wx77,0wx77,0wx77,0wx0,0wxCC,0wx66,0wx66,
   0wx0,0wxDD,0wx0,0wx0,0wx0,0wx55,0wx55,0wx55,0wx0,0wxFF,0wx99,0wx99,0wx0,
   0wxBB,0wxBB,0wxBB,0wx0,0wxFF,0wxCC,0wxCC,0wx0,0wx11,0wx11,0wx11,0wx0,0wxE7,
   0wxE7,0wxE7,0wx0,0wx6F,0wx6F,0wx6F,0wx0,0wxFF,0wx99,0wx0,0wx0,0wx0,0wxFF,
   0wxFF,0wx0,0wxB5,0wxB5,0wx69,0wx0,0wx0,0wxFF,0wx0,0wx0,0wx0,0wx0,0wxFF,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,
   0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0,0wx0]
in

    fun write_bitmap (width:int,height:int,isBlack : int * int -> bool, filename : string) : unit
        =
        let val _ = writebmp (width,height,colormap,
                              fn (x,y) => case isBlack(x,y) of true => 0 | false => 1) filename
        in ()
        end
end

fun writeshape (width,height,s,filename) =
    write_bitmap (width,height,fn p => (contains (s,p)), filename)

fun writeshape_bb (s : shape, filename : string) : unit =
    let val ((minx,miny),(maxx,maxy)) = boundingbox s
    in
        writeshape (maxx-minx,maxy-miny,s,filename)
    end

end
