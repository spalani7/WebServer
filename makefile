CFLAGS = -std=c++11 -Wall -g -O3 -DCACHE  -DHTTP_V11
#CFLAGS = -std=c++11 -Wall -g -O3   -DHTTP_V11
LDFLAGS = -pthread 
CC = g++
SRC = main.cpp HTTPParser.cpp servercache.cpp
HDR = HTTPParser.h log.h servercache.h
OBJ = $(SRC:%.cpp=%.o)
#GDB = gdb --args

server: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp ${HDR}
	$(CC) $(CFLAGS) -c $<

SERVER = 127.0.0.1
PORT = 9000
OUT = output
DIR = ${PWD}
TIMEOUT = 10
WLOG = html/tempfiles/url.log
#RATE = 100 200 300 400 500 1000 2000 3000 4000 5000 
RATE = 100 200 300 400 500 1000 2000 3000 4000 5000 \
       6000 7000 8000 10000 15000 
#RATE =  3000 4000 5000 
#RATE = 10 20 30 40 50
#RATE = 4500

.PHONY: test
test: 
	-mkdir ${OUT}
	python scripts/test.py \
		--timeout ${TIMEOUT} \
		--server ${SERVER} \
		--port  ${PORT} \
		--wlog ${WLOG} \
		--rate ${RATE}

.PHONY: run
run: 
	$(GDB) ./server -dir ${DIR}/html -port ${PORT}

F = 2000
.PHONY: gen
gen:
	cd html && python ../scripts/gen_html.py $F
.PHONY: plot
plot:
	python scripts/plot_results.py ${OUT}/*.txt

.PHONY: plot_final
plot_final:
	python scripts/plot_final.py result.csv

.PHONY: clean
clean:
	rm -rf *.o

.PHONY: clean_all cleanall
cleanall clean_all:
	$(MAKE) -C . clean
	rm -rf server
