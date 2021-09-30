SDSL_DIR		=	sdsl-lite-2.1.1
SDSL_TARGET_DIR	=	sdsl-local-install
ReLZ_DIR		=	ReLZ

DEPENDENCIES	=	$(SDSL_TARGET_DIR)/lib/libsdsl.a \
					$(ReLZ_DIR)/ReLZ

all: $(DEPENDENCIES) lz

.PHONY: all clean lz

clean:
	$(MAKE) -C LZ clean
	rm -rf "sdsl-lite-2.1.1/"
	rm -rf "sdsl-local-install/"
	#rm -rf "sdsl-lite-2.1.1.zip.offline.install.zip"

lz: $(SDSL_TARGET_DIR)/lib/libsdsl.a
	$(MAKE) -C LZ

$(SDSL_TARGET_DIR)/lib/libsdsl.a: $(SDSL_DIR)
	cd $(SDSL_DIR) && \
	./install.sh ../$(SDSL_TARGET_DIR) 

$(SDSL_DIR): sdsl-lite-2.1.1.zip.offline.install.zip
	unzip sdsl-lite-2.1.1.zip.offline.install.zip

sdsl-lite-2.1.1.zip.offline.install.zip:
	wget https://github.com/simongog/sdsl-lite/releases/download/v2.1.1/sdsl-lite-2.1.1.zip.offline.install.zip

# LZscan does not need to be compiled

$(ReLZ_DIR)/ReLZ:
	./get_relz.sh

