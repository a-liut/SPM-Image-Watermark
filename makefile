outname = iw_par
degree = 32
delay = 0

img_root = ./../images/
imgdir = $(img_root)img/
stamp = $(img_root)stamp.jpg
imgdir_big = $(img_root)img_big/
stamp_big = $(img_root)stamp_big.jpg

outprefix = "out_*"
main = main.cpp

main:
	g++ -std=c++11 -O3 -o $(outname) $(main) -lm -I/opt/X11/include -L/usr/X11R6/lib -lpthread -lX11

clean_img:
	find $(imgdir) -name $(outprefix) -exec rm -f {} \;
	find $(imgdir_big) -name $(outprefix) -exec rm -f {} \;

clean:
	rm -f ./$(outname)

cleanall: clean clean_img

exec_t:
	perf stat -d ./$(outname) $(degree) $(imgdir) $(stamp) $(delay)

exec_t_big:
	perf stat -d ./$(outname) $(degree) $(imgdir_big) $(stamp_big) $(delay)

exec:
	./$(outname) $(degree) $(imgdir) $(stamp) $(delay)

exec_big:
	./$(outname) $(degree) $(imgdir_big) $(stamp_big) $(delay)

test_t: main clean_img exec_t

test_t_big: main clean_img exec_t_big

test: main clean_img exec

test_big: main clean_img exec_big

debug:
	g++ -std=c++11 -O3 -g -o $(outname) $(main) -lm -I/opt/X11/include -L/usr/X11R6/lib -lpthread -lX11

run_t: clean_img exec_t

run_t_big: clean_img exec_t_big

run: clean_img exec

run_big: clean_img exec_big

