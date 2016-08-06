open Graphics

val i = IntInf.toInt

val d = 350
val buf1 = 25
val buf = 200

type index = Int32.int
type facet = index list
type sol = point list * facet list * point list

datatype intersect =
         NoInt
         | Source
         | Dest
         | New of rat

fun pointToString (x, y) =
    (Rat.toString x) ^ "," ^ (Rat.toString y)

(* intersect (a, b) edge
   Determines where, if anywhere, the line segment edge intersects ax + b *)
fun intersect (a, b) ((x1, y1), (x2, y2)) =
    let val _ = print ("intersect " ^ (pointToString (x1, y1)) ^ " " ^
                       (pointToString (x2, y2)) ^ "\n")
        val t = Rat.div (Rat.sub (Rat.add (Rat.mult (a, x1), b), y1),
                         (Rat.sub (Rat.sub (y2, y1),
                                   Rat.mult (a, Rat.sub (x2, x1)))))
                handle Div => (~1, 1)
        val _ = print ("t = " ^ (Rat.toString t) ^ "\n")
    in
        if #2 t = 0 then NoInt else
        if Rat.cmp op< (t, (0, 1)) orelse Rat.cmp op> (t, (1,1)) then
            NoInt
        else if Rat.cmp op= (t, (0, 1)) then Source
        else if Rat.cmp op= (t, (1, 1)) then Dest
        else
            New (Rat.reduce t)
                (* (Rat.add (x1, Rat.mult (t, Rat.sub (x2, x1))),
                 Rat.add (y1, Rat.mult (t, Rat.sub (y2, y1)))) *)
    end

exception Invalid

(* interolate linearly between two points using a factor t in [0,1]*)
fun intpt t ((x1, y1), (x2, y2)) =
    (Rat.add (x1, Rat.mult (t, Rat.sub (x2, x1))),
     Rat.add (y1, Rat.mult (t, Rat.sub (y2, y1))))

fun intersect_poly (a, b)
                   ((facet : facet),
                    (source, facets, dest) : sol) : sol =
    let fun edges ps =
            (* Input: a list of points, clockwise or counterclockwise,
               where the first and last points are the same. *)
            case ps of
                p1::p2::[] => [(p1, p2)]
              | p1::p2::t =>
                (p1, p2)::(edges (p2::t))
        val fnexti =
            if List.length source = List.length dest then List.length source
            else raise Invalid
        fun iter es =
            case es of
                [] => ([], [], [])
              | e::r =>
                let val (s', e', d') = iter r
                    val (i1, i2) = e
                    val (s1, s2) = (List.nth (source, i1),
                                    List.nth (source, i2))
                    val (d1, d2) = (List.nth (dest, i1),
                                    List.nth (dest, i2))
                    val nexti =
                        if List.length s' = List.length d'
                        then Int32.+ (fnexti, List.length s')
                        else raise Invalid
                in
                    case intersect (a, b) (d1, d2) of
                        New t =>
                        (print ("new point: " ^ (pointToString
                                                     (intpt t (d1, d2))) ^ "\n");
                        (s' @ [intpt t (s1, s2)], i1::nexti::e',
                         d' @ [intpt t (d1, d2)]))
                      | _ => (s', i1::e', d')
                end
        val (s', e', d') = iter (edges (facet @ [List.hd facet]))
        fun split_poly is (is1, is2) =
            case is of
                [] => (is1, is2)
              | i::t =>
                if Int32.>= (i, fnexti) then
                    split_poly t (i::is2, i::is1)
                else
                    split_poly t (i::is1, is2)
        val (f1, f2) = split_poly e' ([], [])
    in
        if f2 = [] then (source @ s', f1::facets, dest @ d')
        else (source @ s', f1::f2::facets, dest @ d')
    end

(* Reflects point (x,y) across line ax + b *)
fun reflect_pt (a, b) (x, y) =
    let val _ = print ("reflect " ^ (Rat.toString x) ^ ", " ^ (Rat.toString y) ^ "\n")
        val d = Rat.div (Rat.add (x, Rat.mult (Rat.sub (y, b), a)),
                         Rat.add ((1, 1), Rat.mult (a, a)))
        val td = Rat.mult (d, (2, 1))
        val x' = Rat.sub (td, x)
        val y' = Rat.add (Rat.sub (Rat.mult (td, a), y), Rat.mult ((2, 1), b))
        val _ = print ((Rat.toString x') ^ ", " ^ (Rat.toString y') ^ "\n")
    in
        (x', y')
    end

(* Fold sol along line  ax + b*)
fun fold_sol (a, b) (sol as (source, facets, dest) : sol) : sol =
    let val (s', f', d') = List.foldl (intersect_poly (a, b))
                                      sol
                                      facets
        fun maybe_reflect_pt (x, y) =
            if Rat.cmp op< (y, (Rat.add (Rat.mult (a, x), b))) then
                reflect_pt (a, b) (x, y)
            else
                (x, y)
        val d'' = List.map maybe_reflect_pt d'
    in
        (s', f', d'')
    end

fun facet_to_poly (pts: point list) (f: facet) =
    List.map (fn ind => List.nth (pts, ind)) f

val _ = openwindow NONE (i (d * 2 + buf * 2 + buf1 * 2), i (d + buf + buf1))

val _ = setforeground (MLX.fromints 0 0 0)
val _ = MLX.usleep 100000


fun int_of_rat (a, b) = i (Int.div (a * d, b))

fun draw_edge (offx, offy) ((x1, y1), (x2, y2)) =
    let val (x1, y1) = (int_of_rat x1, int_of_rat y1)
        val (x2, y2) = (int_of_rat x2, int_of_rat y2)
        val _ = print ((Int32.toString x1) ^ ", " ^ (Int32.toString y1) ^ "-" ^
                       (Int32.toString x2) ^ ", " ^ (Int32.toString y2) ^ "\n")
    in
        drawline (Int32.+ (offx, x1))
                 (Int32.+ (offy, y1))
                 (Int32.+ (offx, x2))
                 (Int32.+ (offy, y2))
    end

fun draw_poly off ps =
    let fun edges ps =
            (* Input: a list of points, clockwise or counterclockwise,
               where the first and last points are the same. *)
            case ps of
                p1::p2::[] => [(p1, p2)]
              | p1::p2::t =>
                (p1, p2)::(edges (p2::t))
    in
        List.app (draw_edge off) (edges (ps @ [List.hd ps]))
    end

fun draw_sol ((s, f, ds): sol) =
    (List.app (fn f => draw_poly (i buf1, i buf1) (facet_to_poly s f)) f;
     List.app (fn f => draw_poly (i (d + buf + buf1), i buf1)
                                  (facet_to_poly ds f)) f;
     flush ())


val p0 = (0, 1)
val p1 = (1, 1)

val startsol = ([(p0, p0), (p0, p1), (p1, p1), (p1, p0)],
                [[i 0, i 1, i 2, i 3]],
                [(p0, p0), (p0, p1), (p1, p1), (p1, p0)])

infix 5 |>
fun x |> f = f x

val sol0 =
  startsol
|> fold_sol ((0,1),(1,2))

val sol1 = 
  startsol
|> fold_sol ((0,1),(1,2))
|> fold_sol ((0,1),(3,4))
|> fold_sol ((1,1),(1,2))
|> fold_sol ((~1,1),(3,2))

val sol2 =
  startsol
|> fold_sol ((1,2), (1,4))
(*val _ = draw_sol startsol*)
(* val _ = draw_sol (fold_sol ((0, 1), (1, 2)) startsol)*)


val which = sol2

fun i32s (n : Int32.int) =
 if Int32.<(n, (0: Int32.int)) then "-" ^ Int32.toString(Int32.~ n)
 else Int32.toString n

val is = i32s o i

fun rat_to_string (n,d) =
    case d of
	1 => is n
      | _ => is n ^ "/" ^ is d


fun point_to_string (r1,r2) = rat_to_string r1 ^ "," ^ rat_to_string r2

fun points_to_string s =
    List.map point_to_string s |> String.concatWith "\n"

fun source_points_to_string s =
    i32s (List.length s) ^ "\n" ^ points_to_string s

fun facet_to_string f =
    (List.map i32s (List.length f :: f)) |> String.concatWith " "

fun facets_to_string f =
    Int32.toString (List.length f) ^ "\n" ^ (List.map facet_to_string f |> String.concatWith "\n")

fun sol_to_string ((s,f,ds):sol) =
    source_points_to_string s ^ "\n" ^ facets_to_string f ^ "\n" ^ points_to_string ds

fun print_sol ((s,f,ds): sol) =
    print ("******* SOLUTION STARTS HERE *********\n" ^ sol_to_string (s,f,ds) ^ "\n******* SOLUTION ENDS HERE *********\n" )

val _ = draw_sol which

val _ = print_sol which

fun loop () = (MLX.usleep 1000; loop ())

val _ = loop ()
