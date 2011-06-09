#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2011 Serge Monkewitz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License v3 as published
# by the Free Software Foundation, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# A copy of the LGPLv3 is available at <http://www.gnu.org/licenses/>.
#
# Authors:
#     - Serge Monkewitz, IPAC/Caltech
#
# Work on this project has been sponsored by LSST and SLAC/DOE.
#

from __future__ import with_statement
import itertools
import glob
import optparse
import os
import re
import subprocess
import sys
import tempfile
import traceback
try:
    import xml.etree.cElementTree as etree
except:
    import xml.etree.ElementTree as etree
from mako.template import Template
from mako.lookup import TemplateLookup


# -- Helper functions ----

def _extract_content(elt, maybe_empty=False):
    i = len(elt.tag)
    s = etree.tostring(elt, "utf-8")
    i = s.find(">") + 1
    j = s.rfind("<")
    s = s[i:j]
    if not maybe_empty and len(s.strip()) == 0:
        raise RuntimeError('<%s> is empty or contains only whitespace' % elt.tag)
    return s

def _check_attrib(elt, attrib):
    for key in elt.keys():
        if key not in attrib:
            raise RuntimeError('<%s> has illegal attribute %s' % (parent.tag, key))

def _find_one(parent, tag, required=True, attrib=[]):
    elts = parent.findall(tag)
    if elts == None or len(elts) == 0:
        if required:
            raise RuntimeError('<%s> must contain exactly one <%s> child' % (parent.tag, tag))
        return None
    elif len(elts) != 1:
        raise RuntimeError('<%s> contains multiple <%s> children' % (parent.tag, tag))
    _check_attrib(elts[0], set(attrib))
    return elts[0]

def _find_many(parent, tag, required=True, attrib=[]):
    elts = parent.findall(tag)
    if elts == None or len(elts) == 0:
        if required:
            raise RuntimeError('<%s> must contain at least one <%s> child' % (parent.tag, tag))
        return []
    attrib = set(attrib)
    for elt in elts:
        _check_attrib(elt, attrib)
    return elts

def _validate_children(parent, child_tags):
    child_tags = set(child_tags)
    for child in parent:
        if child.tag not in child_tags:
            raise RuntimeError('<%s> cannot contain <%s> children' % (parent.tag, child.tag))


# -- DOM classes for scisql documentation ----

class Description(object):
    """A description of a UDF or stored procedure, extracted from
    the contents of a <desc> tag. The following attributes are available:

    full:   Full UDF/stored procedure description, stored as a unicode string
            containing an XHTML fragment.
    brief:  The first sentence of the full description
    """
    def __init__(self, parent):
        desc = _find_one(parent, 'desc')
        self.full = _extract_content(desc).strip()
        i = self.full.find(".")
        if i == -1: self.brief = re.sub(r'\s+', ' ', self.full)
        else:       self.brief = re.sub(r'\s+', ' ', self.full[:i + 1])


class Note(object):
    """A note, e.g. about UDF usage. The following attributes are available:

    clazz:  The kind of note, e.g. "warning", "info", "error"...
    note:   The note itself, stored as a UTF-8 string containing
            an XHTML fragment. 
    """
    def __init__(self, elt):
        self.clazz = elt.get('class', '')
        self.content = _extract_content(elt).strip()

        
class Example(object):
    """A source code example, e.g. of UDF usage. The following attributes are available:

    lang:   The language of the example source code, typically 'sql' or 'bash'.
    test:   True if the source should be run during example verification.
    source: The example source code, stored as a string containing an
            XHTML fragment.
    """
    def __init__(self, elt):
        self.lang = elt.get('lang', 'sql')
        self.test = elt.get('test', 'true') == 'true'
        s = _extract_content(elt)
        # dedent by the amount of leading whitespace in the first
        # line containing non-whitespace characters
        lines = s.split('\n')
        trim = None
        for i in xrange(len(lines)):
            line = lines[i]
            if len(line.strip()) == 0:
                lines[i] = ''
                continue
            if trim == None:
                trim = re.match(r'\s*', line).group(0)
            if not line.startswith(trim):
                raise RuntimeError('inconsistent leading whitespace in <example> source code')
            lines[i] = line[len(trim):]
        self.source = '\n'.join(lines)


class Argument(object):
    """An argument for a UDF or stored procedure. The following attributes are available:

    kind:        One of 'IN', 'INOUT', or 'OUT' (always 'IN' for a UDF)
    name:        Argument name.
    type:        SQL argument type.
    units:       Expected units, may be None
    brief:       Brief argument description
    description: Full argument description
    """
    def __init__(self, elt):
        self.kind = elt.get('kind', 'IN').upper()
        if self.kind not in ('IN', 'INOUT', 'OUT'):
            raise RuntimeError('<%s> kind attribute value must be "IN", "INOUT", or "OUT"') 
        attr = elt.get('name')
        if attr == None or len(attr.strip()) == 0:
            raise RuntimeError('<%s> missing name attribute' % elt.tag)
        self.name = attr.strip()
        attr = elt.get('type')
        if attr == None or len(attr.strip()) == 0:
            raise RuntimeError('<%s> missing type attribute' % elt.tag)
        self.type = attr.strip()
        self.units = elt.get('units', '')
        self.description = _extract_content(elt).strip()


class ArgumentList(object):
    """An argument list, e.g. for a UDF. The following attributes are available:

    varargs: True if the argument list has a variable number of arguments
    args:    A list of Argument objects
    """
    def __init__(self, elt, attrib):
        _validate_children(elt, ['arg'])
        self.varargs = elt.get('varargs', 'false') == 'true'
        self.args = map(Argument, _find_many(elt, 'arg', required=False, attrib=attrib))


class Udf(object):
    """Documentation for a UDF. The following attributes are available:

    aggregate:   True if this an aggregate UDF
    internal:    True if this UDF is not intended for direct use
    name:        The name of the UDF
    return_type: The return type of the UDF
    section:     The name of the section (category, group) the UDF belongs to

    arglists:    A list of ArgumentList objects for the UDF.
    description: A Description for the UDF.
    examples:    A list of usage Example objects, may be empty.
    notes:       A list of Note objects, may be empty.
    """
    def __init__(self, elt):
        _check_attrib(elt, ['aggregate', 'internal', 'name', 'return_type', 'section'])
        _validate_children(elt, ['desc','notes','args','example'])
        self.aggregate = elt.get('aggregate', 'false') == 'true'
        self.internal = elt.get('internal', 'false') == 'true'
        attr = elt.get('name')
        if attr == None or len(attr.strip()) == 0:
            raise RuntimeError('<udf> element has missing or empty name attribute')
        self.name = attr.strip()
        attr = elt.get('return_type')
        if attr == None or len(attr.strip()) == 0:
            raise RuntimeError('<udf> element has missing or empty return_type attribute')
        self.return_type = attr.strip()
        self.section = elt.get('section', 'misc')
        self.arglists = map(lambda x: ArgumentList(x, ['name', 'type', 'units']),
                            _find_many(elt, 'args', attrib=['varargs'])) 
        self.description = Description(elt)
        self.examples = map(Example, _find_many(elt, 'example', required=False, attrib=['lang', 'test']))
        notes = _find_one(elt, 'notes', required=False)
        if notes == None:
            self.notes = []
        else:
            self.notes = map(Note, _find_many(notes, 'note', attrib=['class']))


class Proc(object):
    """Documentation for a stored procedure. The following attributes are available:

    internal:    True if this procedure is not intended for direct use
    name:        The name of the procedure
    section:     The name of the section (category, group) the procedure belongs to

    args:        A list of Argument objects for the procedure
    description: A Description for the procedure.
    examples:    A list of usage Example objects, may be empty.
    notes:       A list of Note objects, may be empty.
    """
    def __init__(self, elt):
        _check_attrib(elt, ['internal', 'name', 'section'])
        _validate_children(elt, ['desc','notes','args','example'])
        self.internal = elt.get('internal', 'false') == 'true'
        attr = elt.get('name')
        if attr == None  or len(attr.strip()) == 0:
            raise RuntimeError('<proc> element has missing or empty name attribute')
        self.name = attr.strip()
        self.section = elt.get('section', 'misc')
        args = _find_one(elt, 'args', required=False)
        if args == None:
            self.args = []
        else:
            self.args = ArgumentList(args, ['kind', 'name', 'type', 'units']).args
        self.description = Description(elt)
        self.examples = map(Example, _find_many(elt, 'example', required=False, attrib=['lang', 'test']))
        notes = _find_one(elt, 'notes', required=False)
        if notes == None:
            self.notes = []
        else:
            self.notes = map(Note, _find_many(notes, 'note', attrib=['class']))


class Section(object):
    """A documentation section; contains information about a group/category of UDFs,
    possibly including worked examples. The following attributes are available:

    name:     Section name, must not contain spaces
    title:    Section title
    content:  XHTML section content in string form.
    examples: A list of Example objects in the section content, in order of
              occurence.
    """
    def __init__(self, elt):
        attr = elt.get('name')
        if attr == None  or len(attr.strip()) == 0:
            raise RuntimeError('<section> element has missing or empty name attribute')
        self.name = attr.strip()
        attr = elt.get('title')
        if attr == None  or len(attr.strip()) == 0:
            raise RuntimeError('<section> element has missing or empty title attribute')
        self.title = attr.strip()
        self.udfs = []
        self.procs = []
        # Extract example source code
        exlist = list(elt.iter('example'))
        self.examples = map(Example, exlist)
        # Turn <example> tags into <pre> tags with the appropriate prettify attributes
        for ex in exlist:
            ex.tag = 'pre'
            lang = ex.get('lang', 'sql')
            for k in ex.keys():
                del ex.attrib[k]
            ex.set('class', 'prettyprint lang-%s linenums' % lang)
        self.content = _extract_content(elt)


# -- Extracting documentation from source code

def ast(elt):
    if elt.tag == 'udf':
        return Udf(elt)
    elif elt.tag == 'proc':
        return Proc(elt)
    else:
        raise RuntimeError('Unrecognized XML element <%s>' % elt.tag)

def extract_docs_from_c(filename):
    f = open(filename, 'rb')
    text = f.read()
    # Extract comment blocks from file - note that nested comment blocks
    # are not dealt with properly
    comments = []
    beg = text.find("/**")
    while beg != -1:
        end = text.find("*/", beg + 3)
        if end == -1:
            break
        comments.append(text[beg + 3: end].strip())
        beg = text.find("/**", end + 2)
    docs = []
    for block in comments:
        if block.find("</udf>") == -1 and block.find("</proc>") == -1:
            continue
        # Strip leading * from each line in block
        lines = block.split('\n')
        stripped_lines = []
        for line in lines:
            m = re.match(r'\s*\*', line)
            if m != None:
                stripped_lines.append(line[len(m.group(0)):])
            else:
                stripped_lines.append(line)
        xml = '\n'.join(stripped_lines)
        try:
            elt = etree.XML(xml)
            docs.append(ast(elt))
        except:
            print >>sys.stderr, "Failed to parse documentation block:\n\n%s\n\n" % xml
            print >>sys.stderr, traceback.format_exception_only(sys.exc_type, sys.exc_value)
    return docs

def extract_docs_from_sql(filename):
    comments = []
    f = open(filename, 'rb')
    block = '' 
    for line in f:
        m = re.match(r'\s*--', line)
        if m != None:
            block += line[len(m.group(0)):]
        else:
            if len(block) > 0:
                comments.append(block)
            block = ''
    docs = []
    for xml in comments:
        if xml.find("</udf>") == -1 and xml.find("</proc>") == -1:
            continue
        try:
            elt = etree.XML(xml)
            docs.append(ast(elt))
        except:
            print >>sys.stderr, "Failed to parse documentation block:\n\n%s\n\n" % xml
            print >>sys.stderr, traceback.format_exception_only(sys.exc_type, sys.exc_value)
    return docs

def extract_sections(filename):
    elt = etree.parse(filename).getroot()
    if elt.tag != 'sections':
        raise RuntimeError('Root element of a section documentation file must be <section>!')
    return map(Section, _find_many(elt, 'section', attrib=['name', 'title']))

def extract_docs(root):
    nodes = []
    for file in glob.glob(os.path.join(root, 'src', 'udfs', '*.c')):
        nodes.extend(extract_docs_from_c(file))
    for file in glob.glob(os.path.join(root, 'scripts', '*.mysql')):
        nodes.extend(extract_docs_from_sql(file))
    sections = extract_sections(os.path.join(root, 'tools', 'templates', 'sections.xml'))
    secdict = dict((x.name, x) for x in sections)
    for x in nodes:
        if isinstance(x, Udf):
            secdict[x.section].udfs.append(x)
        elif isinstance(x, Proc):
            secdict[x.section].procs.append(x)
    for sec in sections:
        sec.udfs.sort(key=lambda x: x.name)
        sec.procs.sort(key = lambda x: x.name)
    return sections


# -- Testing examples in documentation ----

def _test(obj):
    for ex in obj.examples:
        if not ex.test or ex.lang not in ('sql', 'bash'):
            continue
        with tempfile.TemporaryFile() as source:
            if ex.lang == 'sql':
                source.write('USE scisql_demo;\n\n')
                args = [ os.environ['MYSQL'], '--defaults-file=%s' % os.environ['MYSQL_CNF'] ]
            else:
                args = [ '/bin/bash' ]
            source.write(ex.source)
            source.flush()
            source.seek(0)
            try:
                devnull = open(os.devnull, 'wb')
                subprocess.check_call(args, shell=False, stdin=source, stdout=devnull)
            except:
                print >>sys.stderr, "Failed to run documentation example:\n\n%s\n\n" % ex.source

def run_doc_examples(sections):
    """Runs all examples marked as testable in the SciSQL documentation.
    """
    for sec in sections:
        _test(sec)
        for elt in itertools.chain(sec.udfs, sec.procs):
            _test(elt)


# -- Documentation generation ----

def gen_docs(root, sections, html=True):
    """Generates documentation for SciSQL, either in HTML or as a set of
    MySQL tables (for the LSST schema browser).
    """
    lookup = TemplateLookup(directories=[os.path.join(root, 'tools', 'templates')])
    if html:
        template = lookup.get_template('index.mako')
        f = open(os.path.join(root, 'doc', 'index.html'), 'wb')
        f.write(template.render(sections=sections))
    else:
        template = lookup.get_template('lsst_schema_browser.mako')
        sys.stdout.write(template.render(sections=sections))


# -- Usage and command line processing

usage = """
%prog --help

    Display usage information.

%prog
%prog test

    Make sure code samples in the documentation actually run.

%prog gen_html

    Generate HTML documentation for SciSQL in doc/index.html.

%prog gen_lsst

    Generate documentation in LSST schema browser format and
    write it to standard out.
"""

def main():
    parser = optparse.OptionParser(usage=usage)
    opts, args = parser.parse_args()
    if len(args) > 1 or (len(args) == 1 and args[0] not in ('test', 'gen_html', 'gen_lsst')):
        parser.error("Too many arguments or illegal command")
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
    sections = extract_docs(root)
    if len(args) == 0 or args[0] == 'test':
        run_doc_examples(sections)
    else:
        gen_docs(root, sections, html=(args[0] == 'gen_html'))

if __name__ == '__main__':
    main()

