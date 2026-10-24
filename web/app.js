const $ = (id) => document.getElementById(id);
const player = new Player();
let runProgram = null;
let playing = false;
let timer = null;
let finalNote = null;

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

const delay = () => 1800 / Number($('speed').value);

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
  b.textContent = playing ? '⏸' : '▶';
  b.setAttribute('aria-label', playing ? 'Pause' : 'Play');
}
const manual = (fn) => () => {
    stop();
    fn();
    render();
};

function wireUp() {
  for (const ex of EXAMPLES) {
    const o = document.createElement('option');
    o.textContent = ex.name;
    $('examples').append(o);
  }
  $('examples').onchange = () => {
    $('src').value = EXAMPLES[$('examples').selectedIndex].code;
    execute();
  };

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

function initialProgram() {
  try {
    const fromLink = decodeURIComponent(location.hash.slice(1));
    if (fromLink) return fromLink;
  }
  catch (_) {}
  return EXAMPLES[1].code;
}

async function init() {
  wireUp();
  $('src').value = initialProgram();
  if (typeof createStepper !== 'function') {
    showError('Could not load stepper.js', 'Build it with ./build_wasm.sh, and open this page over http (python3 -m http.server), not file://.');
    return;
  }
  const module = await createStepper();
  runProgram = module.cwrap('run', 'string', ['string']);
  execute();
}
init();