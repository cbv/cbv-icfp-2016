open Graphics

val i = IntInf.toInt

val d = 400
val buf = 100

val file =
    case CommandLine.arguments () of
        f::t => f
      | _ => (print "Usage: vis <problem file>\n";
              OS.Process.exit OS.Process.failure)

val _ = openwindow NONE (i (d * 2 + buf * 4), i (d + buf * 2))

val _ = setforeground (MLX.fromints 0 0 0)
val _ = MLX.usleep 10000

fun int_of_rat (a, b) = i (Int.div (a * d, b))

fun draw_edge (offx, offy) ((x1, y1), (x2, y2)) =
    let val (x1, y1) = (int_of_rat x1, int_of_rat y1)
        val (x2, y2) = (int_of_rat x2, int_of_rat y2)
(*        val _ = print ((Int32.toString x1) ^ ", " ^ (Int32.toString y1) ^ "-" ^
                       (Int32.toString x2) ^ ", " ^ (Int32.toString y2) ^ "\n") *)
    in
        drawline (Int32.+ (offx, x1))
                 (Int32.+ (offy, y1))
                 (Int32.+ (offx, x2))
                 (Int32.+ (offy, y2))
    end

fun draw_skel (off: Int32.int * Int32.int) (s: skelly) =
    List.app (draw_edge off) s

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

fun draw_sil (off: Int32.int * Int32.int) (s: silly) =
    List.app (draw_poly off) s

fun draw_prob ((sil, skel): problem) =
    (draw_sil (i buf, i buf) sil;
     draw_skel (i (d + buf * 3), i buf) skel;
     flush ())

val _ = draw_prob (load file)

fun loop () = (MLX.usleep 1000; loop ())

val _ = loop ()
