SDSL_DIR		=	sdsl-lite-2.1.1
SDSL_TARGET_DIR	=	sdsl-local-install

BWA_DIR			=	BWA/bwa-0.7.12

DEPENDENCIES	=	$(SDSL_TARGET_DIR)/lib/libsdsl.a \
					$(BWA_DIR)/bwa

all: $(DEPENDENCIES) lz

.PHONY: all clean lz

clean:
	$(MAKE) -C $(BWA_DIR) clean
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

$(BWA_DIR)/bwa:
	$(MAKE) -C $(BWA_DIR)
