Bruteforce Search
=================

Approximate Solutions:
----------------------
- "translate the square to cover the silhouette"

- Approximate by folding on all edges of the convex hull of the silhouette (repeatedly).

Exact Solutions that are probably slow:
---------------------------------------
- Exact search: unwrap faces from skeleton.

- Will skeleton be provided as hint with all solutions?

Hard Problems:
--------------
- lots of small faces during search

Research
========
- http://erikdemaine.org/papers/CGTA2000/paper.pdf
  - three general polynomial-time algorithms from strips of unlimited length, not a fixed square
    * the ring method (second one) could potentially be directly applicable
  - a subroutine for convex polygons (Fig 2 & Theorem 2)
    * we should probably implement this. the example can be solved by this algorithm immediately.

Computational Geomotry
======================
- Rational numbers, use really long (greater than 64 bits) integers?
- 2D vector operations
- Mirroring, line intersection, union of multiple polygons
  * Clockwise or not? See Shoelace's formula https://en.wikipedia.org/wiki/Shoelace_formula

Other Properties
================
- A valid solution is probably physically feasible. See http://erikdemaine.org/papers/PaperReachability_CCCG2004/paper.ps
