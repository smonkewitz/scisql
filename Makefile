WAF=python waf

all: sharedlib

sharedlib:
	@$(WAF) build

clean:
	@$(WAF) clean

distclean:
	@$(WAF) distclean

list:
	@$(WAF) list

install:
	@$(WAF) install

uninstall:
	@$(WAF) uninstall

drop:
	@$(WAF) drop

test:
	python -m unittest test
#	@$(WAF) test

.PHONY: all sharedlib clean distclean list install uninstall create drop test

