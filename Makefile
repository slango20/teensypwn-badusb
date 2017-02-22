all:
	platformio run
.PHONY : all
flash:
	platformio run --target=upload
.PHONY : flash
