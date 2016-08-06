signature SHAPE =
sig

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
(*  | Rotate of shape * int*) (* degrees + is counterclockwise*)

val contains : shape * point -> bool
val boundingbox : shape -> point* point
val size : shape -> int*int  (* (width,height) *)
val writeshape : int * int * shape * string -> unit
val writeshape_bb : shape * string -> unit

end
