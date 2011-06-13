<%def name="proc_docs(section, proc)">
	% if proc.internal:
	<div id="proc-${section.name}-${proc.name}" class="proc internal">
		<h3><a name="${section.name}-${proc.name}"></a>[internal] scisql.${proc.name}</h3>
	% else:
	<div id="proc-${section.name}-${proc.name}" class="proc">
		<h3><a name="${section.name}-${proc.name}"></a>scisql.${proc.name}</h3>
	% endif
		<table class="signature">
			<tr>
				<td class="decl">PROCEDURE ${proc.name} (</td>
	% if len(proc.args) > 0:
		% for i, arg in enumerate(proc.args):
			% if i != 0:
			<tr>
				<td></td>
			% endif
				<td class="argtype">${arg.kind}</td>
				<td class="argname">${arg.name}</td>
			% if i != len(proc.args) - 1:
				<td class="argtype">${arg.type},</td>
			% else:
				<td class="argtype">${arg.type}</td>
			% endif
				<td class="argunits">${arg.units}</td>
				<td class="argdesc">${arg.description}</td>
			</tr>
		% endfor
			<tr>
				<td class="decl">)</td>
				<td class="return" colspan="3"></td>
				<td colspan="2"></td>
			</tr>
        % else:
				<td class="return">)</td>
			</tr>
        % endif
		</table>
		<div class="description">
			${proc.description.full}
		</div>
	% if len(proc.notes) > 0:
		<h5>Notes</h5>
		<ul class="notes">
		% for note in proc.notes:
			<li class="${note.clazz}">${note.content}</li>
		% endfor
		</ul>
	% endif
	% if len(proc.examples) > 0:
		<h5>Examples</h5>
		% for example in proc.examples:
		<pre class="prettyprint lang-${example.lang} linenums">${example.source}</pre>
		% endfor
	% endif
	</div>
</%def>
