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

test:
	@$(WAF) test

html_docs:
	@$(WAF) html_docs

dist:
	@$(WAF) dist

.PHONY: all sharedlib clean distclean list install test html_docs dist 
