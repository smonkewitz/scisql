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

load:
	@$(WAF) load

unload:
	@$(WAF) unload

test:
	@$(WAF) test

.PHONY: all sharedlib clean distclean list install uninstall load unload test

