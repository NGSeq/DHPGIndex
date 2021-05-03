all: externals index 
#test_data


externals:
	@echo "Compiling external tools"
	$(MAKE) -C ./ext

index:
	@cd src;make;cd ..;

#test_data:
#	@cd test_scripts;make data;cd ..;

clean:
	@ cd src;make clean;cd ..;
	@ cd ext;./clean.sh;cd ..;
