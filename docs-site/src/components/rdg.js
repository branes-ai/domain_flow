// rdg.js — a self-contained (no-dependency) SVG viewer for a SURE/SARE's Reduced
// Dependency Graph (RDG), issue #103.
//
// The RDG is the recurrence system drawn as a graph: each recurrence VARIABLE becomes
// a NODE, and each dependence between variables becomes an ARC. An arc is annotated
// with the DEPENDENCE MAP it carries:
//   - UNIFORM dependence → a constant TRANSLATION VECTOR, e.g. reading a(i,j,k-1) is
//     the shift [0,0,1]ᵀ (theta = -b);
//   - AFFINE dependence → the MATRIX TRANSFORMATION p ↦ A·p + b (a projection /
//     broadcast), e.g. LU's pivot tap has A projecting (i,j,k) ↦ (k,k,k-1).
// A system whose arcs are all translation vectors is a genuine SURE; any matrix
// (affine) arc makes it a SARE.
//
// Input is the JSON emitted by `dfactl --sure <op>.sure --emit-rdg <out.json>`:
//   { operator, rank, indexNames:["i","j","k"], kind:"SURE"|"SARE",
//     variables:["a","acc", ...],
//     arcs:[ { from, to, kind:"uniform"|"affine",
//              theta?:[...],                 // present for uniform (theta = -b)
//              map:{ A:[[...]], b:[...] } } ] }
//
// Uniform arcs draw solid and are labeled with the translation vector; affine arcs
// draw dashed/accented and are labeled with the matrix map. Used via
// <div class="rdg" data-src="..."> in markdown. Pure SVG + DOM (the docs-site CSP
// forbids external assets); theme-aware via CSS custom properties.

const SVGNS = 'http://www.w3.org/2000/svg';
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => (
  { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

const el = (name, attrs = {}, text) => {
  const n = document.createElementNS(SVGNS, name);
  for (const [k, v] of Object.entries(attrs)) n.setAttribute(k, v);
  if (text != null) n.textContent = text;
  return n;
};

const f2 = (p) => `${p[0].toFixed(1)} ${p[1].toFixed(1)}`;

// The dependence map, as the lines of text that annotate the arc.
//   uniform → one line, the translation vector  [0,0,1]ᵀ
//   affine  → the matrix p ↦ A·p + b, bracketed over `rank` lines, offset on the mid row
function arcLabelLines(arc) {
  if (arc.kind !== 'affine') {
    const theta = arc.theta || (arc.map ? (arc.map.b || []).map((x) => -x) : []);
    return [`[${theta.join(',')}]ᵀ`];
  }
  const A = arc.map?.A || [];
  const b = arc.map?.b || [];
  const r = A.length;
  const rows = A.map((row) => row.map((x) => String(x).padStart(2)).join('  '));
  const lines = r <= 1
    ? [`[${rows[0] || ''}]`]
    : rows.map((row, i) => {
        const lb = i === 0 ? '⎡' : (i === r - 1 ? '⎣' : '⎢');
        const rb = i === 0 ? '⎤' : (i === r - 1 ? '⎦' : '⎥');
        return `${lb} ${row} ${rb}`;
      });
  // the offset b goes on its OWN line: every matrix row is then equal width, so the
  // bracket columns stay aligned under the centered text anchor (appending b to the
  // middle row would widen it and shift that row's centre off the others)
  if (b.some((x) => x !== 0)) lines.push(`+ [${b.join(',')}]ᵀ`);
  return lines;
}
const arcTitle = (arc) => arc.kind === 'affine'
  ? 'affine dependence (SARE): the matrix map p ↦ A·p + b'
  : 'uniform dependence (SURE): the translation vector θ';

// Build one RDG viewer inside `container` from parsed `data`.
export function createRdgViewer(container, data) {
  const vars = Array.isArray(data.variables) ? data.variables : [];
  const arcs = Array.isArray(data.arcs) ? data.arcs : [];
  const n = vars.length;

  const W = 640, H = Number(container.getAttribute('data-height')) || 620;
  const cx = W / 2, cy = H / 2;
  // Ring radius: push the nodes well out from the centre so cross-node arcs have room
  // for their dependence-map labels. A taller canvas (default 620) grows this radius
  // ~50% over a square one while the 165px margin still holds the outer loop + its label.
  const R = Math.max(60, Math.min(cx, cy) - 165);
  const NR = 34;                                     // node circle radius

  // node positions: single node centered; otherwise evenly on a ring, starting at top
  const pos = {};
  vars.forEach((v, i) => {
    if (n === 1) { pos[v] = [cx, cy]; return; }
    const a = -Math.PI / 2 + (2 * Math.PI * i) / n;
    pos[v] = [cx + R * Math.cos(a), cy + R * Math.sin(a)];
  });

  // A node's self-loop sits on the OUTSIDE of the graph — it points away from the
  // centroid of all the nodes — so it never overlaps the node cluster and leaves room
  // for its annotation. On a ring layout that is the radially-outward direction; a node
  // with neighbours above/below/left ends up looping to its right, one boxed in above
  // loops underneath, and so on, all as consequences of "point away from the crowd".
  let gcx = 0, gcy = 0;
  for (const v of vars) { gcx += pos[v][0]; gcy += pos[v][1]; }
  gcx /= Math.max(1, n); gcy /= Math.max(1, n);
  const outwardAngle = (v) => {
    const dx = pos[v][0] - gcx, dy = pos[v][1] - gcy;
    return Math.hypot(dx, dy) < 1e-3 ? null : Math.atan2(dy, dx);   // null ⇒ node is the centroid
  };

  const svg = el('svg', {
    viewBox: `0 0 ${W} ${H}`, class: 'rdg-svg',
    preserveAspectRatio: 'xMidYMid meet', role: 'img',
    'aria-label': `Reduced dependency graph for ${esc(data.operator || 'operator')}`,
  });
  // An inline SVG with a viewBox but no intrinsic height collapses to zero under
  // `height:auto`. Pin the aspect ratio so the height follows the (100%) width.
  svg.style.aspectRatio = `${W} / ${H}`;
  svg.style.height = 'auto';
  svg.style.minHeight = `${Math.round((H / W) * 320)}px`;

  // arrowhead markers (uniform + affine tint)
  const defs = el('defs');
  for (const [id, cls] of [['rdg-ah', 'rdg-uniform'], ['rdg-ah-aff', 'rdg-affine']]) {
    const m = el('marker', {
      id, class: cls, markerWidth: '9', markerHeight: '9', refX: '7.5', refY: '3',
      orient: 'auto', markerUnits: 'strokeWidth',
    });
    m.append(el('path', { d: 'M0,0 L8,3 L0,6 Z' }));
    defs.append(m);
  }
  svg.append(defs);

  const edgeLayer = el('g', { class: 'rdg-edges' });
  const labelLayer = el('g', { class: 'rdg-labels' });
  const nodeLayer = el('g', { class: 'rdg-nodes' });
  svg.append(edgeLayer, labelLayer, nodeLayer);

  // arc labels, collected as drawn so backing rects can be sized after layout (getBBox).
  // Declared BEFORE the arc loop that calls addLabel(): a `const` is in the temporal
  // dead zone until its declaration runs, so a later declaration throws at the first arc.
  const labels = [];

  const SEP = '\u0000';   // key separator: variable names can't contain a NUL
  // Group arcs so that every arc between a given UNORDERED node pair shares one set of
  // parallel channels. Grouping by the directed (from,to) instead lays each direction out
  // independently: reciprocal arcs (a→r and r→a) measure their offset against OPPOSITE
  // edge normals, so a→r's channel +k lands on the same physical curve as r→a's channel −k
  // and the two draw on top of each other — lstsq's a↔r has four arcs (two each way) but
  // only two channels appeared. Keying by the sorted endpoint pair puts all four in one list;
  // a canonical normal + round-robin offsets then hand each arc its own path. Self-dependences
  // (from === to) group per node and fan out as outward loops instead.
  const groups = new Map();
  for (const arc of arcs) {
    const key = arc.from === arc.to
      ? `self${SEP}${arc.from}`
      : `pair${SEP}${[arc.from, arc.to].sort().join(SEP)}`;
    if (!groups.has(key)) groups.set(key, []);
    groups.get(key).push(arc);
  }

  // A self-loop bulging OUTWARD in direction φ. The two anchors sit close together on
  // the rim (±aAng straddling φ); the two control points are pushed out to radius cR at
  // a slightly wider angle (±cAng), so the belly balloons OUTSIDE the node (never behind
  // it). cR ≈ 2.9·NR puts the belly ~2·NR from the centre — a loop about twice the size
  // it needs so the arrowhead clears the arc instead of merging into it. The arrowhead
  // re-enters at the second anchor; the label goes just past the belly, along φ.
  function selfLoopPath(center, phi) {
    const aAng = 0.34;          // anchor half-angle (mouth on the rim)
    const cAng = 0.54;          // control half-angle (a touch wider ⇒ the loop opens)
    const cR = NR * 2.9;        // control radius ⇒ belly ≈ 2.1·NR from centre (~1·NR past the rim)
    const u = (ang, r) => [center[0] + r * Math.cos(ang), center[1] + r * Math.sin(ang)];
    const P0 = u(phi - aAng, NR), P3 = u(phi + aAng, NR);
    const C1 = u(phi - cAng, cR), C2 = u(phi + cAng, cR);
    const lab = u(phi, cR + 20);
    return { d: `M ${f2(P0)} C ${f2(C1)} ${f2(C2)} ${f2(P3)}`, lab };
  }

  for (const [key, list] of groups) {
    const [kind, u, w] = key.split(SEP);
    if (kind === 'self') {
      const P = pos[u];
      if (!P) continue;
      const K = list.length;
      const base = outwardAngle(u);
      list.forEach((arc, k) => {
        const cls = arc.kind === 'affine' ? 'rdg-affine' : 'rdg-uniform';
        const marker = arc.kind === 'affine' ? 'url(#rdg-ah-aff)' : 'url(#rdg-ah)';
        // point the loop away from the crowd; a node's several self-loops fan around
        // that outward direction, and a lone node spreads its loops evenly all round
        const phi = base === null
          ? -Math.PI / 2 + (2 * Math.PI * k) / K
          : base + (k - (K - 1) / 2) * 0.62;
        const { d, lab } = selfLoopPath(P, phi);
        edgeLayer.append(el('path', { class: `rdg-edge ${cls}`, 'marker-end': marker, fill: 'none', d }));
        addLabel(lab[0], lab[1], arc);
      });
      continue;
    }
    // Non-self pair: give each arc its OWN channel, round-robin. Offsets are measured against
    // a normal fixed by the pair's canonical (sorted u,w) orientation, so both directions index
    // the SAME channel set and no two arcs — either direction — land on the same curve.
    const P = pos[u], Q = pos[w];
    if (!P || !Q) continue;
    const mx = (P[0] + Q[0]) / 2, my = (P[1] + Q[1]) / 2;
    const dx = Q[0] - P[0], dy = Q[1] - P[1];
    const len = Math.hypot(dx, dy) || 1;
    const nx = -dy / len, ny = dx / len;                   // canonical unit normal (both directions)
    const M = list.length;
    // Channel offsets, symmetric about the node-to-node line. An AFFINE arc carries a
    // multi-line matrix map (p ↦ A·p + b) that needs room to render; a uniform arc's label
    // is a one-line translation vector. So hand the affine arcs the OUTERMOST channels —
    // largest |offset|, belly out in open peripheral space — and let the uniform arcs take
    // the inner channels. Assigning strictly by insertion order (as before) could bury a
    // matrix map on an inner channel between two other arcs: the QR-Givens (lstsq) graph
    // was unreadable even once the arcs no longer overlapped.
    const slots = Array.from({ length: M }, (_, k) => (k - (M - 1) / 2) * 40)
      .sort((p, q) => Math.abs(q) - Math.abs(p) || p - q);  // outermost first; neg before pos
    // stable order: affine arcs first (→ outer slots), then uniform (→ inner)
    const order = [...list.keys()].sort((a, b) => {
      const aff = (i) => (list[i].kind === 'affine' ? 0 : 1);
      return aff(a) - aff(b) || a - b;
    });
    const offOf = new Array(M);
    order.forEach((arcIdx, slotIdx) => { offOf[arcIdx] = slots[slotIdx]; });
    list.forEach((arc, k) => {
      const cls = arc.kind === 'affine' ? 'rdg-affine' : 'rdg-uniform';
      const marker = arc.kind === 'affine' ? 'url(#rdg-ah-aff)' : 'url(#rdg-ah)';
      const off = offOf[k];                                 // one distinct channel per arc
      const c = [mx + nx * off, my + ny * off];
      // draw between the arc's ACTUAL endpoints so the arrowhead points the true way; only
      // the channel (control point) comes from the canonical orientation
      const A = pos[arc.from], B = pos[arc.to];
      const s = trim(A, c, NR), e = trim(B, c, NR + 4);
      edgeLayer.append(el('path', {
        class: `rdg-edge ${cls}`, 'marker-end': marker, fill: 'none',
        d: `M ${f2(s)} Q ${f2(c)} ${f2(e)}`,
      }));
      // Outboard label placement (issue #143): sit each map OUTSIDE its own arc and grow
      // it AWAY from the graph, so reciprocal maps never stack and a matrix never straddles
      // the arc. `off`'s sign gives the outward direction along the channel normal; the
      // normal's orientation says whether that side is left/right (vertical node line) or
      // above/below (horizontal node line). A centered anchor would push half the label
      // back over the arcs — instead anchor start/end (grow left/right) or align the block
      // fully above/below. Affine maps (multi-line matrix) get a bit more clearance.
      const sgn = Math.sign(off);
      const ox = nx * sgn, oy = ny * sgn;                 // outward unit vector for this arc
      const gap = off === 0 ? 0 : (arc.kind === 'affine' ? 16 : 8);
      const place = off === 0 ? {}
        : (Math.abs(ox) >= Math.abs(oy)
            ? { anchor: ox > 0 ? 'start' : 'end' }        // maps go left / right
            : { vAlign: oy > 0 ? 'below' : 'above' });    // maps go above / below
      addLabel(c[0] + ox * gap, c[1] + oy * gap, arc, place);
    });
  }

  function trim(A, ctrl, r) {
    const dx = ctrl[0] - A[0], dy = ctrl[1] - A[1];
    const d = Math.hypot(dx, dy) || 1;
    return [A[0] + (dx / d) * r, A[1] + (dy / d) * r];
  }

  // A (possibly multi-line) arc label anchored at (x, y). `place.anchor` (start|middle|end)
  // grows the text left/right; `place.vAlign` (above|middle|below) puts the whole block
  // above or below y instead of centered — together they let the caller push a map fully
  // outboard of its arc (issue #143). Defaults reproduce the old centered placement.
  function addLabel(x, y, arc, place = {}) {
    const cls = arc.kind === 'affine' ? 'rdg-affine' : 'rdg-uniform';
    const anchor = place.anchor || 'middle';
    const lines = arcLabelLines(arc);
    const lh = 13;
    const n = lines.length;
    // startDy = baseline offset of the FIRST line relative to y (subsequent lines +lh)
    const startDy = place.vAlign === 'above' ? -((n - 1) * lh) - 4   // block ends just above y
      : place.vAlign === 'below' ? 14                                 // block starts just below y
      : -((n - 1) / 2) * lh + 4;                                      // centered on y
    const g = el('g', { class: `rdg-label ${cls}` });
    g.append(el('title', {}, arcTitle(arc)));
    const t = el('text', { x: x.toFixed(1), y: y.toFixed(1), 'text-anchor': anchor });
    lines.forEach((ln, i) => {
      t.append(el('tspan', { x: x.toFixed(1), dy: (i === 0 ? startDy : lh).toFixed(1) }, ln));
    });
    g.append(t);
    labelLayer.append(g);
    labels.push({ g, t });
  }

  // nodes on top
  for (const v of vars) {
    const [x, y] = pos[v];
    const g = el('g', { class: 'rdg-node' });
    g.append(el('circle', { cx: x, cy: y, r: NR }));
    g.append(el('text', { x, y, 'text-anchor': 'middle', dy: '0.32em' }, v));
    nodeLayer.append(g);
  }

  container.append(svg);

  // measure labels and add a backing rect so edges don't run through the text
  for (const { g, t } of labels) {
    try {
      const bb = t.getBBox();
      const pad = 3;
      const rect = el('rect', {
        class: 'rdg-label-bg',
        x: (bb.x - pad).toFixed(1), y: (bb.y - pad).toFixed(1),
        width: (bb.width + 2 * pad).toFixed(1), height: (bb.height + 2 * pad).toFixed(1),
        rx: '3',
      });
      g.insertBefore(rect, t);
    } catch { /* getBBox unavailable (jsdom) — skip backing */ }
  }

  return svg;
}

// ── global mounter: turn every <div class="rdg" data-src> into an SVG graph ──
export async function mountAll(root = document) {
  const nodes = root.querySelectorAll('.rdg[data-src]:not([data-mounted])');
  for (const elm of nodes) {
    elm.setAttribute('data-mounted', '1');
    const raw = elm.getAttribute('data-src');
    const base = (import.meta.env.BASE_URL || '/').replace(/\/$/, '');
    const src = /^https?:/.test(raw) ? raw : `${base}/${raw.replace(/^\//, '')}`;

    let data;
    try {
      const r = await fetch(src);
      if (!r.ok) throw new Error(`${r.status}`);
      data = await r.json();
    } catch (err) {
      const w = document.createElement('div');
      w.className = 'rdg-warn';
      w.textContent = `failed to load ${src}: ${err.message}`;
      elm.append(w);
      continue;
    }

    createRdgViewer(elm, data);

    // caption: operator + SURE/SARE classification + a small legend
    const cap = document.createElement('div');
    cap.className = 'rdg-legend';
    const kind = data.kind === 'SARE' ? 'SARE' : 'SURE';
    cap.innerHTML =
      `<span class="rdg-kind rdg-kind-${kind.toLowerCase()}">${esc(kind)}</span>` +
      `<span><i class="rdg-swatch rdg-uniform"></i>uniform arc — translation vector</span>` +
      `<span><i class="rdg-swatch rdg-affine"></i>affine arc — matrix map A·p + b</span>`;
    elm.append(cap);
  }
}
