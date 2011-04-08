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
	@$(WAF) test

dist:
	@$(WAF) dist


.PHONY: all sharedlib clean dist distclean list install uninstall create drop test

