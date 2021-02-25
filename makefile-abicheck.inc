ABI_CHECK_LIBNAME := $(patsubst %.so,%,$(LIBFILE))

abi-check:
	$(MAKE) abi-check-clean
	$(MAKE)	abi-compliance-check
	
rev1-tree: abi-check-dir
	if [ -z "$(rev1)" ] ; then \
	    echo "make abi-check: mandatory parameters rev1 and(or) rev2 mising"; \
	    false; \
	else \
	    git archive --prefix=$(rev1)/ --format tar $(rev1): | tar x -C abi-check; \
	fi

rev2-tree: abi-check-dir
	if [ -z "$(rev2)" ] ; then \
	    echo "make abi-check: mandatory parameters rev1 and(or) rev2 mising"; \
	    false; \
	else \
	    git archive --prefix=$(rev2)/ --format tar $(rev2): | tar x -C abi-check; \
	else

abi-check-dir:
	mkdir -p abi-check

rev1-build: rev1-tree
	$(MAKE) -C abi-check/$(rev1) debug

rev2-build: rev2-tree
	$(MAKE) -C abi-check/$(rev2) debug

rev1-abi-dump: rev1-build
	abi-dumper abi-check/$(rev1)/$(ABI_CHECK_LIBNAME).so -o abi-check/$(ABI_CHECK_LIBNAME)-$(rev1).dump --lver $(rev1)

rev2-abi-dump: rev2-build
	abi-dumper abi-check/$(rev2)/$(ABI_CHECK_LIBNAME).so -o abi-check/$(ABI_CHECK_LIBNAME)-$(rev2).dump --lver $(rev2)

abi-compliance-check: rev1-abi-dump rev2-abi-dump
	abi-compliance-checker \
	    -l $(ABI_CHECK_LIBNAME) \
	    -old abi-check/$(ABI_CHECK_LIBNAME)-$(rev1).dump \
	    -new abi-check/$(ABI_CHECK_LIBNAME)-$(rev2).dump 2>&1 | tee abi-check/result.txt
	    
abi-check-clean:
	rm -rf abi-check

.PHONY:	abi-check rev1-tree rev2-tree
