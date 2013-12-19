TARGET := libmax_clique_mpi.a

SOURCES := \
	mpi_max_clique.cc

TGT_CXX := ${MPICXX}
TGT_LINKER := ${MPICXX}

