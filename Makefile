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

test:
	@$(WAF) test

test_docs:
	@$(WAF) test_docs

html_docs:
	@$(WAF) html_docs

lsst_docs:
	@$(WAF) lsst_docs

dist:
	@$(WAF) dist


.PHONY: all sharedlib clean dist distclean list install uninstall create test test_docs html_docs lsst_docs

