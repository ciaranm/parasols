TARGET := create_graph_product

SOURCES := create_graph_product.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lgraph $(boost_ldlibs)
TGT_PREREQS := libgraph.a

