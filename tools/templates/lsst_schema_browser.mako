<%!
_sql_escapes = [ ('\\', '\\\\'), ('"', '\\"'), ('\t', '\\t'), ('\r', '\\r'), ('\n', '\\n') ]

def sql_escape(text):
    for c,rep in _sql_escapes:
        text = text.replace(c, rep)
    return text
%>
<%namespace file="udf.mako" import="udf_docs"/>
<%namespace file="proc.mako" import="proc_docs"/>

-- ! ! !    W A R N I N G    ! ! !
-- This file is auto-generated - do NOT update it by hand!

DROP DATABASE IF EXISTS lsst_schema_browser_scisql;
CREATE DATABASE lsst_schema_browser_scisql;
USE lsst_schema_browser_scisql;


CREATE TABLE md_Udf (
	udfId INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
	name VARCHAR(255) NOT NULL UNIQUE,
	category VARCHAR(255) NOT NULL,
	htmlDocs TEXT,
        INDEX md_Udf_name (name)
);

CREATE TABLE md_Proc (
        procId INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
        name VARCHAR(255) NOT NULL UNIQUE,
	dbName VARCHAR(255) NOT NULL,
        category VARCHAR(255) NOT NULL,
        htmlDocs TEXT,
        INDEX md_Proc_name (name)
);


-- Populate md_Udf ----

% for section in sections:
	% for udf in section.udfs:
INSERT INTO md_Udf SET
	name = "${udf.name | sql_escape}",
	category = "${section.title | h,sql_escape}",
	htmlDocs = "${capture(udf_docs, section, udf) | sql_escape}";

	% endfor
% endfor


-- Populate md_Proc ----

% for section in sections:
	% for proc in section.procs:
INSERT INTO md_Proc SET
	name = "${proc.name | sql_escape}",
	dbName = "scisql",
	category = "${sql_escape(section.title) | h,sql_escape}",
	htmlDocs = "${capture(proc_docs, section, proc) | sql_escape}";

	% endfor
% endfor

