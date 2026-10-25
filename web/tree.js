const SVG_NS = 'http://www.w3.org/2000/svg';
const GAP = 14, LEVEL = 78, NODE_H = 30;

function svgEl(name, attrs, parent) {
  const n = document.createElementNS(SVG_NS, name);
  for (const k in attrs) n.setAttribute(k, attrs[k]);
  if (parent) parent.append(n);
  return n;
}

function labelOf(n) {
  switch (n.k) {
    case 'lam': return '\u03bb' + n.t;
    case 'let': return 'let ' + n.t;
    case 'letrec': return 'let rec ' + n.t;
    case 'app': return 'apply';
    case 'if': return 'if';
    case 'map': return 'map';
    case 'fold': return 'fold';
    case 'list': return (n.c ? '[' + n.c.length + ']' : '[ ]');
    case 'ref': return n.t + ' \u21ba';
    case 'neg': return 'neg';
    default: return n.t;
  }
}

class TreeView {
  constructor(svg, wrap) {
    this.svg = svg;
    this.wrap = wrap;
    this.s = 1; this.tx = 0; this.ty = 0;
    this._userMoved = false;
    this.following = false;
    this.hotNode = null;
    this.data = null;
    this.bad = false;
    this.size = { w: 0, h: 0 };
    this.g = null;
    this.bind();
  }

  bind() {
    const svg = this.svg;
    svg.addEventListener('wheel', (e) => {
      e.preventDefault();
      const f = e.deltaY < 0 ? 1.15 : 1 / 1.15;
      if (this.following) {
        this.rescale(f);
        return;
      }
      const r = svg.getBoundingClientRect();
      this.zoomAt(e.clientX - r.left, e.clientY - r.top, f);
    }, { passive: false });

    let drag = null;
    svg.addEventListener('pointerdown', (e) => {
      drag = { x: e.clientX, y: e.clientY, tx: this.tx, ty: this.ty };
      svg.setPointerCapture(e.pointerId);
      svg.classList.add('grabbing');
    });
    svg.addEventListener('pointermove', (e) => {
      if (!drag) return;
      this.tx = drag.tx + e.clientX - drag.x;
      this.ty = drag.ty + e.clientY - drag.y;
      this.following = false;
      this.userMoved = true;
      this.apply();
    });
    const end = () => { drag = null; svg.classList.remove('grabbing'); };
    svg.addEventListener('pointerup', end);
    svg.addEventListener('pointercancel', end);

    new ResizeObserver(() => {
      if (this.wrap.hidden || !this.data) return;
      if (this.following) this.centerOnHot();
      else if (!this.userMoved) this.fitTransform();
    }).observe(this.wrap);
  }

  get userMoved() { return this._userMoved; }
  set userMoved(v) {
    this._userMoved = v;
    if (!v) this.following = false;
  }

  zoomAt(mx, my, f) {
    const ns = Math.max(0.05, Math.min(4, this.s * f));
    this.tx = mx - (mx - this.tx) * ns / this.s;
    this.ty = my - (my - this.ty) * ns / this.s;
    this.s = ns;
    this.following = false;
    this.userMoved = true;
    this.apply();
  }
  zoomBy(f) {
    if (this.following) {
      this.rescale(f);
      return;
    }
    const r = this.svg.getBoundingClientRect();
    this.zoomAt(r.width / 2, r.height / 2, f);
  }
  rescale(f) {
    this.s = Math.max(0.05, Math.min(4, this.s * f));
    this.centerOnHot();
  }

  fit() {
    this.userMoved = false;       // also clears following
    this.fitTransform();
  }

  fitTransform() {
    const cw = this.svg.clientWidth, ch = this.svg.clientHeight;
    if (!cw || !ch || !this.size.w) return;
    const s = Math.min((cw - 24) / this.size.w, (ch - 24) / this.size.h, 1.5);
    this.s = s;
    this.tx = (cw - this.size.w * s) / 2;
    this.ty = (ch - this.size.h * s) / 2;
    this.apply();
  }

  focus() {
    if (!this.hotNode) return;
    this.following = true;
    this._userMoved = true;
    if (this.s < 0.8) this.s = 1;
    this.centerOnHot();
  }

  centerOnHot() {
    const h = this.hotNode;
    const cw = this.svg.clientWidth, ch = this.svg.clientHeight;
    if (!h || !cw || !ch) return;
    this.tx = cw / 2 - h.x * this.s;
    this.ty = ch * 0.4 - (h.y + h.h / 2) * this.s;
    this.apply();
  }

  apply() {
    this.wrap.classList.toggle('following', this.following);
    if (this.g) this.g.setAttribute('transform', `translate(${this.tx} ${this.ty}) scale(${this.s})`);
  }

  show(tree, bad) {
    this.data = tree;
    this.bad = !!bad;
    this.draw();
  }

  layout(n) {
    n.label = labelOf(n);
    n.w = Math.max(44, 8 * n.label.length + 22);
    n.h = NODE_H;
    const kids = n.c || [];
    kids.forEach((k) => this.layout(k));
    const total = kids.reduce((s, k) => s + k.span, 0) + GAP * Math.max(0, kids.length - 1);
    n.span = Math.max(n.w, total);
    n.total = total;
  }

  place(n, left, depth) {
    n.y = depth * LEVEL;
    this.maxDepth = Math.max(this.maxDepth, depth);
    const kids = n.c || [];
    if (!kids.length) {
      n.x = left + n.span / 2;
      return;
    }
    let x = left + (n.span - n.total) / 2;
    for (const k of kids) {
      this.place(k, x, depth + 1);
      x += k.span + GAP;
    }
    n.x = (kids[0].x + kids[kids.length - 1].x) / 2;
  }

  draw() {
    this.svg.replaceChildren();
    this.g = null;
    this.hotNode = null;
    if (!this.data) return;

    this.maxDepth = 0;
    this.layout(this.data);
    this.place(this.data, 0, 0);
    this.size = { w: this.data.span, h: this.maxDepth * LEVEL + NODE_H };

    this.g = svgEl('g', {}, this.svg);
    const edges = svgEl('g', {}, this.g);
    const nodes = svgEl('g', {}, this.g);
    this.walk(this.data, edges, nodes, false);

    if (this.following) {
      if (this.hotNode) this.centerOnHot();
      else this.fitTransform();
    }
    else if (this.userMoved) this.apply();
    else this.fitTransform();
  }

  walk(n, edges, nodes, inHot) {
    if (n.hot) this.hotNode = n;
    inHot = inHot || !!n.hot;
    for (const c of n.c || []) {
      const x1 = n.x, y1 = n.y + n.h, x2 = c.x, y2 = c.y, ym = (y1 + y2) / 2;
      const onPath = n.hot ? false : (c.hot || c.on || inHot);
      svgEl('path', {
        d: `M${x1} ${y1} C${x1} ${ym},${x2} ${ym},${x2} ${y2}`,
        class: 'te' + (onPath ? ' on' : ''),
      }, edges);
      if (c.r) {
        const t = svgEl('text', { x: x2, y: y2 - 6, class: 'role' }, edges);
        t.textContent = c.r;
      }
    }
    const cls = ['tn', 'k-' + n.k];
    if (n.hot) cls.push('hot');
    if (n.on) cls.push('on');
    if (inHot) cls.push('in-hot');
    if (this.bad && inHot) cls.push('bad');
    const g = svgEl('g', { class: cls.join(' ') }, nodes);
    svgEl('rect', { x: n.x - n.w / 2, y: n.y, width: n.w, height: n.h, rx: 8 }, g);
    const t = svgEl('text', { x: n.x, y: n.y + n.h / 2, 'text-anchor': 'middle', 'dominant-baseline': 'central' }, g);
    t.textContent = n.label;
    for (const c of n.c || []) this.walk(c, edges, nodes, inHot);
  }
}