ifeq ($(prism-backend),up-down)
PRISM_BACKEND=up-down
else ifeq ($(prism-backend),sr)
PRISM_BACKEND=sr
else
$(error prism-backend is not set (must be up-down or sr))
endif

ifndef optimization
$(error optimization is not set)
endif

ifdef debug
DEBUG=--show-cmd \
	--verbose \
	--save-temps \
	--prism-backend-debug \
	--prism-backend-debug=abi \
	--prism-backend-debug=getoperands  
endif

ifdef size
vector_size=-DSIZE=$(size)
BINARY=test_$(optimization)_$(size)
SOURCE=test_vector.cpp
else
BINARY=test_$(optimization)
SOURCE=test_scalar.cpp
endif

CXX=verificarlo-c++

CFLAGS=-g -Wall \
	$(optimization) \
	$(vector_size) \
	-Wno-psabi \
	-Wno-unknown-warning-option \
	--prism-backend=${PRISM_BACKEND} \
	$(DEBUG)

LDFLAGS=--prism-backend=${PRISM_BACKEND} \
	-lm \
	$(DEBUG)

ifdef static
CFLAGS+=--prism-backend-dispatch=static
LDFLAGS+=--prism-backend-dispatch=static
endif

ifdef native
CFLAGS+=-march=native
endif

all:
	@mkdir -p .objects .bin
	$(CXX) $(CFLAGS) -c $(SOURCE) -o .objects/$(BINARY).o
	$(CXX) .objects/$(BINARY).o -o .bin/$(BINARY) $(LDFLAGS) 
