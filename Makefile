hdmi.bin.uf2: hdmi.bin
	./bin2uf2.py hdmi.bin

hdmi.bin: hdmi.elf
	llvm-objcopy -O binary hdmi.elf hdmi.bin

hdmi.elf: hdmi.o boot.o
	clang++ @pico-cxx-base/link_flags.txt \
		-o hdmi.elf \
		hdmi.o boot.o

hdmi.o: hdmi.s
	clang++ @pico-cxx-base/compile_flags.txt -xassembler -c -o hdmi.o hdmi.s

hdmi.s: hdmi.cc
	clang++ @pico-cxx-base/compile_flags.txt -xc++ -c -S -o hdmi.s hdmi.cc

boot.o: boot.s
	clang++ @pico-cxx-base/compile_flags.txt -xassembler -c -o boot.o boot.s

boot.s: pico-cxx-base/picobase/boot.cc
	clang++ @pico-cxx-base/compile_flags.txt -xc++ -c -S -o boot.s pico-cxx-base/picobase/boot.cc

clean:
	rm -f hdmi.o hdmi.s hdmi.bin hdmi.elf