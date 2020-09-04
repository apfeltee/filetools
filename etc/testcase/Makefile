
src       = main.cpp
out_cyg   = cygtest.exe
out_clang = clangtest.exe
out_vs    = vstest.exe

all: $(out_cyg) $(out_clang) $(out_vs)

$(out_cyg): $(src)
	g++ -g3 -ggdb $(src) -lboost_system -lboost_filesystem -o$(out_cyg)

$(out_clang): $(src)
	clang++ $(src) -o$(out_clang)

$(out_vs): $(src)
	cl $(src) -Fe$(out_vs)

clean:
	rm -f $(out_cyg) $(out_clang) $(out_vs)

rebuild: clean all
