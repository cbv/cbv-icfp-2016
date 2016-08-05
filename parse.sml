val example =
"1" ^ "\n" ^
"4" ^ "\n" ^
"0,0" ^ "\n" ^
"1,0" ^ "\n" ^
"1/2,1/2" ^ "\n" ^
"0,1/2" ^ "\n" ^
"5" ^ "\n" ^
"0,0 1,0" ^ "\n" ^
"1,0 1/2,1/2" ^ "\n" ^
"1/2,1/2 0,1/2" ^ "\n" ^
"0,1/2 0,0" ^ "\n" ^
"0,0 1/2,1/2"

structure Int = IntInf
open IntInf
type rat = int * int
type point = rat * rat
type polygon = point list
type edge = point * point
type skelly = edge list
type silly = polygon list
type problem = silly * skelly

fun assert p = if p then () else raise Fail "extra lines"

fun parse_rat (s:string): rat =
  case String.tokens (fn c => c = #"/") s of
    [s] => (valOf (Int.fromString s), 1)
  | [s1,s2] => (valOf (Int.fromString s1), valOf(Int.fromString s2))

fun parse_vert (ss: string list): point * string list =
  let
    val (vert:: rest) = ss
    val [r1, r2] = String.tokens (fn c => c = #",") vert
  in
    ((parse_rat r1, parse_rat r2), rest)
  end

fun parse_verts (nVerts:int, rest: string list): polygon * string list =
  case nVerts of
    0 => ([], rest)
  | _ =>
    let 
      val (vert, rest) = parse_vert rest
      val (verts, rest) = parse_verts (nVerts - 1, rest)
    in
      (vert::verts, rest)
    end

fun parse_poly (rest:string list): polygon * string list =
  let
    val (nVerts :: rest) = rest
    val nVerts = valOf(Int.fromString nVerts)
  in
    parse_verts (nVerts, rest)
  end

fun parse_polys (n:int, rest:string list):(polygon list)*(string list) =
  case n of
    0 => ([], rest)
  | _ =>
    let
      val (poly, rest) = parse_poly rest
      val (polys, rest) = parse_polys (n-1, rest)
    in
      (poly::polys, rest)
    end

fun parse_silly (lines:string list):silly*(string list) =
  let
    val (nPoly:: rest) = lines
    val nPoly = valOf (Int.fromString nPoly)
  in
    parse_polys (nPoly, rest)
  end

fun parse_seg (rest:string list):edge * string list =
  let
    val (seg::rest) = rest
    val [p1,p2] = String.tokens (fn c => c = #" ") seg
    val ((p1,[]),(p2,[])) = (parse_vert [p1], parse_vert [p2])
  in
   ((p1,p2),rest)
  end

fun parse_segs (nSegs:int, rest:string list): skelly * string list =
  case nSegs of
    0 => ([], rest)
  | _ =>
    let
      val (seg, rest) = parse_seg rest
      val (segs, rest) = parse_segs (nSegs - 1, rest)
    in
      (seg::segs, rest)
    end

fun parse_skelly (lines:string list): skelly * string list =
  let
    val (nSegs::rest) = lines
    val nSegs = valOf (Int.fromString nSegs)
  in
    parse_segs (nSegs, rest)
  end
  
fun parse (str:string):problem =
  let
    val (silly, rest) = parse_silly (String.tokens (fn c => c = #"\n") str)
    val (skelly, rest) = parse_skelly rest
    val () = assert (rest = [])
  in
    (silly, skelly)
  end

val load = parse o TextIO.inputAll o TextIO.openIn