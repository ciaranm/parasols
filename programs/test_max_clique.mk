TARGET := test_max_clique

SOURCES := test_max_clique.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -Wl,--no-as-needed -lmax_clique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_clique.a libgraph.a libthreads.a

