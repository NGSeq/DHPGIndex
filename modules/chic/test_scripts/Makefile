EINSTEIN_DIR				=	utest_data/large

DATA_DEPENDENCIES =	$(EINSTEIN_DIR)/einstein.en.txt \

data: $(DATA_DEPENDENCIES)
	@echo ""
	@echo "Large test file in place."

all: data
.PHONY: all

$(EINSTEIN_DIR)/einstein.en.txt:
	cd $(EINSTEIN_DIR); wget wget http://pizzachili.dcc.uchile.cl/repcorpus/real/einstein.en.txt.7z; p7zip -d einstein.en.txt.7z;

