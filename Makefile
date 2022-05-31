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

deploy:
	@$(WAF) deploy

deploy_sql:
	@$(WAF) deploy_sql

uninstall:
	@$(WAF) uninstall

test:
	@$(WAF) test

html_docs:
	@$(WAF) html_docs

dist:
	@$(WAF) dist


.PHONY: all sharedlib clean dist distclean list install install_sql uninstall create test html_docs

