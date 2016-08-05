structure Rat =
struct

fun gcd (a, b) =
    let val _ = if a <= 0 orelse b <= 0 then raise Div else ()
        (* val _ = print ("gcd (" ^ (Int.toString a) ^ ", " ^
                       (Int.toString b) ^ ")\n") *)
    in
    if a < b then
        gcd (b, a)
    else
        let val (q, r) = divMod (a, b)
        in
            if r = 0 then b
            else
                gcd (b, r)
        end
    end

fun lcm (a, b) =
    Int.div (a * b, gcd (a, b))

fun reduce (a, b) =
    if a = 0 then (0, 1)
    else
        let val _ = if b = 0 then raise Div else ()
            val (a, b) = if b < 0 then (~a, ~b) else (a, b)
            val d = if a < 0 then gcd (~a, b)
                         else gcd (a, b)
        in
            (Int.div (a, d), Int.div (b, d))
        end

fun min ((a1, b1), (a2, b2)) =
    if b1 = b2 then
        (IntInf.min (a1, a2), b1)
    else
        if a1 * b2 < a2 * b1 then
            (a1, b1)
        else
            (a2, b2)

fun add ((a1, b1), (a2, b2)) =
    let val d = lcm (b1, b2)
        val a1' = a1 * (Int.div (d, b1))
        val a2' = a2 * (Int.div (d, b2))
    in
        (a1' + a2', d)
    end

fun neg (a, b) = (~a, b)

fun sub (a, b) =
    add (a, neg b)

fun mult ((a1, b1), (a2, b2)) =
    reduce (a1 * a2, b1 * b2)

fun div ((a1, b1), (a2, b2)) =
    reduce (a1 * b2, b1 * a2)

fun cmp oper ((a1, b1), (a2, b2)) =
    let val d = lcm (b1, b2)
        val a1' = a1 * (Int.div (d, b1))
        val a2' = a2 * (Int.div (d, b2))
    in
        oper (a1', a2')
    end

fun toString (a, b) =
    if b = 1 then Int.toString a
    else (Int.toString a) ^ "/" ^ (Int.toString b)

end
