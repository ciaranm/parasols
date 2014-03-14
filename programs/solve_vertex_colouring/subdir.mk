TARGET := solve_vertex_colouring

SOURCES := solve_vertex_colouring.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lvertex_colour -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libvertex_colour.a libgraph.a

