PROG1=	bin/cl
PROG2=	bin/srv

OBJS1=	udmuser_cl.o	
OBJS2=	udmuser_srv.o
CFLAGS=	-Wall -Werror -Wextra -g

PP=	g++ 
VER=	"-std=c++20"

.cpp.o:
	${PP} ${VER} ${CFLAGS} -c $< -o $@

all: ${PROG1} ${PROG2}

udmuser_cl.o: gateway.h session.h presentation.h datafactory.h worker_pool.hpp stdutils.h userclass.h circular_pipeline.hpp
udmuser_srv.o: gateway.h session.h presentation.h datafactory.h worker_pool.hpp stdutils.h userclass.h circular_pipeline.hpp

${PROG1}: ${OBJS1}
	@echo $@ depends on $<
	${PP} ${VER} ${OBJS1} -o ${PROG1} ${LDFLAGS}

${PROG2}: ${OBJS2}
	@echo $@ depends on $<
	${PP} ${VER} ${OBJS2} -o ${PROG2} ${LDFLAGS}

clean:
	rm -f ${PROG1} ${PROG2} ${OBJS1} ${OBJS2}
