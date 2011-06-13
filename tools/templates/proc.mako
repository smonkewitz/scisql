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
				<td class="decl" colspan="5">PROCEDURE ${proc.name} (
	% if len(proc.args) > 0:
				</td>
			</tr>
		% for i, arg in enumerate(proc.args):
			<tr>
				<td class="argkind">${arg.kind}</td>
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
				<td class="decl" colspan="5">)</td>
			</tr>
        % else:
				)</td>
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
