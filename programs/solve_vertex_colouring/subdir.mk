TARGET := solve_vertex_colouring

SOURCES := solve_vertex_colouring.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lvertex_colouring -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libvertex_colouring.a libgraph.a libthreads.a libsolver.a

