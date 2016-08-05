val example =
"1
4
0,0
1,0
1/2,1/2
0,1/2
5
0,0 1,0
1,0 1/2,1/2
1/2,1/2 0,1/2
0,1/2 0,0
0,0 1/2,1/2"

open IntInf
type rat = int * int
type point = rat * rat
type polygon = point list
type edge = point * point
type skelly = edge list
type silly = polygon list
type problem = silly * skelly

fun assert p = if p then () else raise Fail "extra lines"

fun parse_poly (rest:string list): polygon * string list =
  let
    val (nVerts :: rest) =

fun parse_polys (n:int, rest:string list):(polygon list)*(string list) =
  case n of
    0 => ([], rest)
  | _ =>
    let
      val (poly, rest) = parse_poly rest
      val (polys, rest) = parse_polys (n-1) rest
    in
      (poly::polys, rest)
    end

fun parse_silly (lines:string list):silly*(string list) =
  let
    val (nPoly, rest) = lines
    val nPoly = valOf (Int.fromString nPoly)
  in
    parse_polys (n, rest)
  end
  
fun parse (str:string):problem =
  let
    val (silly, rest) = parse_silly (String.tokens str)
    val (skelly, rest) = parse_skelly rest
    val () = assert (rest = [])
  in
    (silly, skelly)
  end