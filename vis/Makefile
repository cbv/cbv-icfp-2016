vis: vis.sml vis.mlb
	mlton -default-ann 'allowFFI true' -link-opt -L/usr/X11R6/lib -link-opt -lX11 -output vis vis.mlb x/mlx.o 

probgen: probgen.sml probgen.mlb ../rat.sml
	mlton -default-ann 'allowFFI true' -const 'Exn.keepHistory true' -link-opt -L/usr/X11R6/lib -link-opt -lX11 -output probgen probgen.mlb x/mlx.o 