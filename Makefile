# Default target
all: mygit

mygit:
	g++ -std=c++20 -o mygit  init.cpp log.cpp cat.cpp main.cpp hash.cpp add.cpp writetree.cpp commit.cpp lstree.cpp checkout.cpp status.cpp show.cpp reset.cpp utilities.cpp -lssl -lcrypto -lz

# Clean up generated files
# clean:
# 	rm -f mygit *.o
#     rm -rf .mygit 