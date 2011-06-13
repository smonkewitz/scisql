<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<%namespace file="udf.mako" import="udf_docs"/>
<%namespace file="proc.mako" import="proc_docs"/>
<%def name="section_link(section)">
	<h3><a href="#${section.name}">${section.title | h}</a></h3>
</%def>
<%def name="section_nav(section)">
	% if len(section.udfs) > 0:
		% if len(section.procs) > 0:
		<h4>Functions</h4>
		% endif
		<ul class="section_nav">
		% for fun in section.udfs:
			<li><a href="#${section.name}-${fun.name}" title="${fun.description.brief}">${fun.name}</a></li>
		% endfor
		</ul>
	% endif
	% if len(section.procs) > 0:
		% if len(section.udfs) > 0:
		<h4>Procedures</h4>
		% endif
		<ul class="section_nav">
		% for proc in section.procs:
			<li><a href="#${section.name}-${proc.name}" title="${proc.description.brief}">${proc.name}</a></li>
		% endfor
		</ul>
	%endif
</%def>

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<title>sciSQL ${SCISQL_VERSION} Documentation</title>
	<link href="docs.css" type="text/css" rel="stylesheet" />
	<link href="prettify/prettify.css" type="text/css" rel="stylesheet" />
	<script type="text/javascript" src="prettify/prettify.js"></script>
	<script type="text/javascript" src="jquery-1.6.1.min.js"></script>
</head>

<body>
<div id="header">sciSQL ${SCISQL_VERSION}: Science Tools for MySQL</div>
<div id="index">Index</div>
<div id="title"></div>

<div id="nav">
<ul>
% for section in sections:
	<li>
		${section_link(section)}
		${section_nav(section)}
	</li>
% endfor
</ul>
</div> <!-- end of #nav -->


<div id="content">
        <!-- section anchors -->
% for section in sections:
        <a name="${section.name}"></a>
%endfor

% for section in sections:
<div id="section-${section.name}" class="section">
	${section.content}

	% if len(section.udfs) > 0:
	<h2>Functions</h2>
		% for udf in section.udfs:
	${udf_docs(section, udf)}
		% endfor
	% endif
	% if len(section.procs) > 0:
	<h2>Procedures</h2>
		% for proc in section.procs:
	${proc_docs(section, proc)}
		% endfor
	% endif
</div> <!-- end of #section-${section.name} -->

% endfor
</div> <!-- end of #content -->

<script type="text/javascript"><!--
	$(document).ready(prettyPrint);
	$(document).ready(function () {
		$('#content a[name]').each(function() {
			$(this).css('position', 'relative').css('top', '-90px').html('&nbsp;');
		});
		$('#nav a').each(function() {
			$(this).click(function() {
				var href = $(this).attr("href").substring(1);
				var loc = href.split('-');
				var section = loc[0];
				$('#nav a.active').removeClass('active');
				$(this).addClass('active');
				var seclink = $('#nav a[href="#' + section + '"]');
				$('#title').text(seclink.text());
				if (loc.length > 1) {
					seclink.addClass('active');
				}
				section = $('#section-' + section);
				if (!section.is(':visible')) {
					$('#content div.section:visible').hide();
					section.fadeIn(300);
				}
			});
		});
		var _loc = null;
		var _hashchange = function() {
			var l = document.location.toString();
			if (l == _loc) {
				return;
			}
			_loc = l;
			var i = l.indexOf('#');
			var e = (i != -1) ? $('#nav a[href="' + l.substring(i) + '"]') : $('#nav a[href="#overview"]');
			e.click();
		};
		_hashchange();
		if ('onhashchange' in window) {
			window.onhashchange = _hashchange;
		} else {
			setInterval(_hashchange, 100);
		}
	});
--></script>
</body>
</html>

