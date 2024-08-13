hdmi.bin.uf2: hdmi.bin
	./bin2uf2.py hdmi.bin

hdmi.bin: hdmi.elf
	llvm-objcopy -O binary hdmi.elf hdmi.bin

hdmi.elf: hdmi.o
	clang++ @link_flags.txt -o hdmi.elf hdmi.o

hdmi.o: hdmi.s
	clang++ @compile_flags.txt -xassembler -c -o hdmi.o hdmi.s

hdmi.s: hdmi.cc
	clang++ @compile_flags.txt -xc++ -c -S -o hdmi.s hdmi.cc
