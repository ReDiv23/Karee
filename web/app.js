const $ = (id) => document.getElementById(id);
const player = new Player();
let runProgram = null;
let playing = false;
let timer = null;
let finalNote = null;
let view = 'code';
const treeView = new TreeView($('tree'), $('treeWrap'));

const ICONS = {
  first: 'M3 5h2v14H3zM13 5v14L5 12zM21 5v14L13 12z',
  prev:  'M6 5h2v14H6zM19 5v14L8 12z',
  play:  'M8 5v14l11-7z',
  pause: 'M7 5h4v14H7zM13 5h4v14h-4z',
  next:  'M6 5v14l11-7zM17 5h2v14h-2z',
  last:  'M3 5v14l8-7zM11 5v14l8-7zM19 5h2v14h-2z',
};
const icon = (name) =>
  '<svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor" aria-hidden="true"><path d="' + ICONS[name] + '"/></svg>';

function setView(v) {
  view = v;
  $('viewCode').setAttribute('aria-pressed', String(v === 'code'));
  $('viewTree').setAttribute('aria-pressed', String(v === 'tree'));
  render();
}

function execute() {
  stop();
  hideError();
  const src = $('src').value;
  try {
    history.replaceState(null, '', '#' + encodeURIComponent(src));
  }
  catch (_) {}

  let result;
  try {
    result = JSON.parse(runProgram(src));
  }
  catch (e) {
    showError('Internal error', String(e));
    $('stage').hidden = $('controls').hidden = true;
    return;
  }

  if (!result.ok && result.error.kind === 'parse') {
    showParseError(src, result.error);
    $('stage').hidden = $('controls').hidden = true;
    return;
  }

  finalNote = null;
  if (result.error) finalNote = { text: 'Runtime error: ' + result.error.msg, cls: 'bad' };
  else if (result.truncated) finalNote = { text: 'Stopped early: this program may never finish.', cls: 'warn' };

  player.load(result.steps);
  $('stage').hidden = $('controls').hidden = false;
  render();
}

function render() {
  const s = player.current;
  if (!s) return;

  const parts = splitAroundRedex(s.text, s.redex);
  const expr = $('expr');
  expr.replaceChildren();
  expr.append(parts.before);
  if (parts.mid) {
    const mark = document.createElement('mark');
    mark.textContent = parts.mid;
    if (s.rule === 'error') mark.className = 'bad';
    expr.append(mark);
  }
  expr.append(parts.after);

  $('expr').hidden = view === 'tree';
  $('treeWrap').hidden = view !== 'tree';
  if (view === 'tree') treeView.show(s.tree, s.rule === 'error');

  const badge = $('rule');
  badge.textContent = s.rule;
  badge.className = 'badge rule-' + s.rule;
  $('caption').textContent = s.caption;

  renderBoxes(s);

  const banner = $('banner');
  if (finalNote && player.atEnd) {
    banner.textContent = finalNote.text;
    banner.className = 'banner ' + finalNote.cls;
    banner.hidden = false;
  }
  else {
    banner.hidden = true;
  }

  $('scrub').max = player.count - 1;
  $('scrub').value = player.i;
  $('counter').textContent = 'Step ' + player.i + ' / ' + (player.count - 1);
  $('first').disabled = $('prev').disabled = player.atStart;
  $('next').disabled = $('last').disabled = player.atEnd;
}

function syncItems(container, texts, isLit, isBad) {
  while (container.children.length < texts.length) {
    const d = document.createElement('div');
    d.className = 'box';
    container.append(d);
  }
  while (container.children.length > texts.length) container.lastChild.remove();
  texts.forEach((t, i) => {
    const el = container.children[i];
    if (el.textContent !== t) el.textContent = t;
    el.title = t;
    el.classList.toggle('lit', isLit(i));
    el.classList.toggle('bad', isLit(i) && isBad);
  });
}

function renderBoxes(step) {
  const b = step.boxes;
  const wrap = $('boxes');
  if (!b) {
    wrap.hidden = true;
    return;
  }
  wrap.hidden = false;

  const bad = step.rule === 'error';
  const acc = $('acc');
  acc.hidden = b.kind !== 'fold';

  if (b.kind === 'list') {
    $('boxLabel').textContent = step.rule === 'done' ? 'result' : 'list';
    syncItems($('items'), b.items, (i) => i === b.active, bad);
  }
  else if (b.kind === 'map') {
    $('boxLabel').textContent = 'map  ' + b.fn;
    syncItems($('items'), b.items, () => b.all, bad);
  }
  else {
    $('boxLabel').textContent = 'fold  ' + b.fn;
    $('accText').textContent = b.acc;
    acc.title = b.acc;
    acc.classList.toggle('lit', b.accActive);
    acc.classList.toggle('absorbing', b.active >= 0);
    acc.classList.toggle('bad', b.accActive && bad);
    syncItems($('items'), b.items, (i) => i === b.active, bad);
  }
}

function showError(title, detail, snippet) {
  const el = $('error');
  el.replaceChildren();
  const h = document.createElement('div');
  h.className = 'error-title';
  h.textContent = title;
  el.append(h);
  if (detail) {
    const p = document.createElement('div');
    p.textContent = detail;
    el.append(p);
  }
  if (snippet) {
    const pre = document.createElement('pre');
    pre.textContent = snippet;
    el.append(pre);
  }
  el.hidden = false;
}
function hideError() { $('error').hidden = true; }

function showParseError(src, err) {
  const line = src.split('\n')[err.line - 1] || '';
  const caret = ' '.repeat(Math.max(0, err.col - 1)) + '^';
  showError('Parse error at line ' + err.line + ', column ' + err.col, err.msg, line + '\n' + caret);
}

const delay = () => 600 / Number($('speed').value);

function play() {
  if (player.atEnd) {
    player.goto(0);
    render();
  }
  playing = true;
  syncPlayButton();
  schedule();
}
function schedule() {
  timer = setTimeout(() => {
    player.next();
    render();
    if (player.atEnd) stop();
    else schedule();
  }, delay());
}
function stop() {
  playing = false;
  clearTimeout(timer);
  syncPlayButton();
}
function syncPlayButton() {
  const b = $('play');
  b.innerHTML = icon(playing ? 'pause' : 'play');
  b.setAttribute('aria-label', playing ? 'Pause' : 'Play');
}
const manual = (fn) => () => {
    stop();
    fn();
    render();
};

const library = EXAMPLES.map((e, i) => ({ id: 'ex' + i, name: e.name, code: e.code, builtin: true }));
let selectedId = 'ex1';
let customCount = 0;
let renaming = false;

const current = () => library.find((p) => p.id === selectedId);

function freshName() {
  const name = (n) => (n === 1 ? 'Custom' : 'Custom ' + n);
  let n = 1;
  while (library.some((p) => p.name === name(n))) n++;
  return name(n);
}

function addCustom(code, name) {
  customCount++;
  const p = { id: 'c' + customCount, name: name || freshName(), code, builtin: false };
  library.push(p);
  selectedId = p.id;
}

function rebuildSelect() {
  const sel = $('examples');
  sel.replaceChildren();
  const groups = [['My programs', library.filter((p) => !p.builtin)], ['Examples', library.filter((p) => p.builtin)]];
  for (const [label, items] of groups) {
    if (!items.length) continue;
    const g = document.createElement('optgroup');
    g.label = label;
    for (const p of items) {
      const o = document.createElement('option');
      o.value = p.id;
      o.textContent = p.name;
      g.append(o);
    }
    sel.append(g);
  }
  sel.value = selectedId;
  $('rename').disabled = $('remove').disabled = current().builtin;
}

function choose(id) {
  selectedId = id;
  $('src').value = current().code;
  rebuildSelect();
  execute();
}

function onEdit() {
  const text = $('src').value;
  const p = current();
  if (p.builtin) {
    if (text === p.code) return;
    addCustom(text);
    rebuildSelect();
  } else {
    p.code = text;
  }
}

function startRename() {
  if (current().builtin) return;
  renaming = true;
  $('nameInput').value = current().name;
  $('examples').hidden = true;
  $('nameInput').hidden = false;
  $('nameInput').focus();
  $('nameInput').select();
}

function newProgram() {
  addCustom('');
  $('src').value = '';
  rebuildSelect();
  $('src').focus();
}

function saveAs() {
  const name = (prompt('Name for the copy:', freshName()) || '').trim();
  if (!name) return;
  addCustom($('src').value, name);
  rebuildSelect();
}

function endRename(save) {
  if (!renaming) return;
  renaming = false;
  const name = $('nameInput').value.trim();
  if (save && name) current().name = name;
  $('nameInput').hidden = true;
  $('examples').hidden = false;
  rebuildSelect();
  $('rename').focus();
}

function removeCurrent() {
  const p = current();
  if (p.builtin) return;
  if (!confirm('Delete "' + p.name + '"?')) return;
  library.splice(library.indexOf(p), 1);
  choose('ex1');
}

function wireUp() {
  for (const name of ['first', 'prev', 'next', 'last']) $(name).innerHTML = icon(name);
  syncPlayButton();
  $('examples').onchange = () => choose($('examples').value);
  $('src').addEventListener('input', onEdit);
  $('rename').onclick = startRename;
  $('remove').onclick = removeCurrent;
  $('new').onclick = newProgram;
  $('saveas').onclick = saveAs;
  $('nameInput').addEventListener('keydown', (e) => {
    if (e.key !== 'Enter' && e.key !== 'Escape') return;
    e.preventDefault();
    endRename(e.key === 'Enter');
  });
  $('nameInput').addEventListener('blur', () => endRename(true));

  $('run').onclick = execute;
  $('src').addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
      e.preventDefault();
      execute();
    }
  });

  $('first').onclick = manual(() => player.goto(0));
  $('prev').onclick  = manual(() => player.prev());
  $('next').onclick  = manual(() => player.next());
  $('last').onclick  = manual(() => player.goto(player.count - 1));
  $('play').onclick  = () => (playing ? stop() : play());
  $('scrub').oninput = manual(() => player.goto(Number($('scrub').value)));

  $('viewCode').onclick = () => setView('code');
  $('viewTree').onclick = () => setView('tree');
  $('zin').onclick = () => treeView.zoomBy(1.25);
  $('zout').onclick = () => treeView.zoomBy(1 / 1.25);
  $('zfit').onclick = () => treeView.fit();

  document.addEventListener('keydown', (e) => {
    if (['TEXTAREA', 'SELECT', 'INPUT'].includes(e.target.tagName)) return;
    if (!player.count) return;
    if (e.key === 'ArrowRight') manual(() => player.next())();
    else if (e.key === 'ArrowLeft') manual(() => player.prev())();
    else if (e.key === 'Home') manual(() => player.goto(0))();
    else if (e.key === 'End') manual(() => player.goto(player.count - 1))();
    else if (e.key === ' ') {
      e.preventDefault();
      playing ? stop() : play();
      }
  });
}

function programFromLink() {
  try { return decodeURIComponent(location.hash.slice(1)); }
  catch (_) { return ''; }
}

async function init() {
  wireUp();
  const linked = programFromLink();
  if (linked) {
    const known = library.find((p) => p.code === linked);
    if (known) selectedId = known.id;
    else addCustom(linked);
  }
  rebuildSelect();
  $('src').value = current().code;
  if (typeof createKaree !== 'function') {
    showError('Could not load karee.js', 'Build it with ./build_wasm.sh, and open this page over http (python3 -m http.server), not file://.');
    return;
  }
  const module = await createKaree();
  runProgram = module.cwrap('run', 'string', ['string']);
  execute();
}
init();