// schedule-anim.js — a framework-agnostic Three.js viewer that animates a SURE's
// schedule: the wavefront sweeping through the index-space lattice, one timestep
// at a time, so parallelism (wavefront width) and latency read at a glance.
//
// Modeled on cortex's docs-site/src/components/scene3d.js. Input is the JSON
// emitted by `dfactl --sure <op>.sure --emit-schedule <out.json>`:
//   { operator, rank, indexNames, bounds:{lo,hi}, schedule:{kind,tau?}, latency,
//     variables:[ {name, points:[{p:[i,j,k], t}]} ] }
//
// Each (variable, point) is an *activation*; it fires at its signature time t.
// At frame f: t < f already fired (solid), t == f the firing wavefront (bright,
// enlarged), t > f pending (faint). The wavefront width at f is the parallelism.
//
// Used via a bare <div class="schedule-anim" data-src="..."> in markdown; the
// global mounter (mountAll) builds the canvas + controls and drives it.

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

// Per-variable colours — the "result" variable (last, e.g. matmul's c) reads green,
// echoing the classic domain-flow linear-schedule picture.
const PALETTE = [0x4fd1ff, 0xff9f0a, 0x35c759, 0xbf5af2, 0xffd60a, 0xff6482, 0x64d2ff];

// Escape values that originate in the fetched schedule JSON before they reach the
// HUD's innerHTML — data-src may be an arbitrary (even https) URL, so a compromised
// schedule must not be able to inject markup into the docs origin.
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => (
  { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

/**
 * Mount the viewer.
 * @param {{canvas:HTMLCanvasElement, hud?:HTMLElement, data:object, options?:object}} cfg
 * @returns {{setFrame:(f:number)=>void, frameCount:number, play:()=>void, pause:()=>void, fitView:()=>void, dispose:()=>void}}
 */
export function createScheduleViewer({ canvas, hud, data, options = {} }) {
  const rank = data.rank ?? (data.bounds?.lo?.length || 3);
  const lo = data.bounds?.lo ?? [0, 0, 0];
  const hi = data.bounds?.hi ?? [1, 1, 1];
  const pad = (p) => [p[0] ?? 0, rank > 1 ? (p[1] ?? 0) : 0, rank > 2 ? (p[2] ?? 0) : 0];
  const ctr = [0, 1, 2].map((k) => ((lo[k] ?? 0) + (hi[k] ?? 0)) / 2);
  const span = Math.max(1, ...[0, 1, 2].map((k) => (hi[k] ?? 0) - (lo[k] ?? 0)));

  // ── tiling overlay (issue #70): when options.tile is given, colour each
  //    activation by the tile block it lands in and draw the block boundaries,
  //    so a domain-exceeds-the-fabric partition reads at a glance. tile is a
  //    per-dim block size (a single number is broadcast to every axis). The
  //    recurrence is unchanged — tiling is a partition of the SAME index space,
  //    which is exactly why a uniform operator tiles. ──────────────────────────
  // accept a tile spec only if it is exactly one size (broadcast to every axis)
  // or three positive finite sizes — reject anything malformed (e.g. "5,,10")
  // rather than silently mis-applying a size to the wrong axis.
  // tile sizes are index-space block lengths → positive integers. Requiring
  // Number.isInteger(n) && n >= 1 also rejects subnormals/fractions like 5e-324
  // that would otherwise make ceil(span/ts) overflow to Infinity in tileOf().
  const tileValid = (t) => Array.isArray(t) && (t.length === 1 || t.length === 3)
    && t.every((n) => Number.isInteger(n) && n >= 1);
  // a bare number is the documented "broadcast to every axis" form → normalize to
  // the one-element array the rest of the code (ts(d) = tileSizes[d] ?? [0]) expects
  const tileArg = typeof options.tile === 'number' ? [options.tile] : options.tile;
  const tileSizes = tileValid(tileArg) ? tileArg : null;
  const tileMode = !!tileSizes;
  // ── dependency-arrow overlay (issue #142, Phase 3): when options.edges is set
  //    and we're NOT in tile mode, replay each variable's taps to draw the
  //    producer→consumer dependence for the firing wavefront — the equation taps
  //    (constant offsets / affine maps) as arrows between lattice points. Reuses
  //    the same tap-replay + per-frame bucketing as the tile overlay, so only the
  //    wavefront's traffic draws each frame. Tile mode already draws its own
  //    (cross-tile) edges, so the two overlays are mutually exclusive. ──────────
  // ── legality overlay (issue #142, Phase 3): replay the taps and colour each dependence
  //    by its slack Δt = t(consumer) − t(producer). A schedule is LEGAL iff every dependence
  //    has Δt ≥ 1 (the producer fires strictly before the consumer); Δt ≤ 0 is a violation
  //    (red). Among the legal ones, Δt = 1 is a latency-binding "tight" dependence (bright
  //    green) and Δt > 1 has slack (dim green). Shares the edge machinery below and is
  //    mutually exclusive with the tile / edge overlays. ────────────────────────────────
  const legalityMode = !!options.legality && !tileMode;
  const edgesMode = !!options.edges && !tileMode && !legalityMode;
  // ── τ-normal wavefront plane (issue #142, Phase 3): a translucent plane normal to the
  //    scheduling vector τ, positioned at the firing wavefront and swept through the
  //    lattice as time advances — the classic domain-flow linear-schedule picture (points
  //    sequenced by their signature t = τ·p). Only meaningful for a linear schedule: a
  //    free schedule has no single τ, so the plane is suppressed there. ─────────────────
  const tauArr = Array.isArray(data.schedule?.tau) ? data.schedule.tau.map(Number) : null;
  // Project τ to 3-D (dims ≥ rank contribute 0), then require EVERY used component to be
  // finite before enabling the plane — validating just one (`.some`) would let a stray
  // non-finite in another dimension (e.g. τ = [1, Infinity, 0]) turn the plane on and then
  // produce NaN coordinates once τ̂ is normalized.
  const tau3Arr = [0, 1, 2].map((d) => (d < rank ? tauArr?.[d] : 0));
  const planeMode = !!options.plane && tau3Arr.every(Number.isFinite) && tau3Arr.some((n) => n !== 0);
  const ts = (d) => tileSizes[d] ?? tileSizes[0];
  const nTiles = [0, 1, 2].map((d) => tileMode ? Math.max(1, Math.ceil(((hi[d] ?? 0) - (lo[d] ?? 0) + 1) / ts(d))) : 1);
  const tileOf = (p) => [0, 1, 2].map((d) => Math.floor(((p[d] ?? 0) - (lo[d] ?? 0)) / ts(d)));
  const tileId = (p) => { const [I, J, K] = tileOf(p); return I + J * nTiles[0] + K * nTiles[0] * nTiles[1]; };
  // one well-separated hue per block via the golden-angle sequence
  const tileColor = (id) => new THREE.Color().setHSL((id * 0.6180339887) % 1, 0.62, 0.56);

  // ── flatten activations, find the time range ──────────────────────────────
  const vars = data.variables ?? [];
  const acts = [];
  let tMin = Infinity, tMax = -Infinity;
  vars.forEach((v, vi) => {
    for (const pt of v.points) {
      acts.push({ p: pad(pt.p), raw: pt.p, t: pt.t, vi });
      tMin = Math.min(tMin, pt.t); tMax = Math.max(tMax, pt.t);
    }
  });
  if (!Number.isFinite(tMin)) { tMin = 0; tMax = 0; }
  const frameCount = tMax - tMin + 1;

  // ── renderer / scene / camera ─────────────────────────────────────────────
  // preserveDrawingBuffer keeps the rendered frame readable via canvas.toDataURL after
  // compositing — the offline capture path (make-video.mjs) reads pixels that way. The cost
  // is negligible for these small scenes and it's otherwise invisible to interactive use.
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, preserveDrawingBuffer: true });
  renderer.setPixelRatio(Math.min(globalThis.devicePixelRatio || 1, 2));
  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0x0e1116);
  const camera = new THREE.PerspectiveCamera(50, 1, 0.01, 1000);
  camera.up.set(0, 0, 1);
  const controls = new OrbitControls(camera, canvas);
  controls.enableDamping = true;
  scene.add(new THREE.AmbientLight(0xffffff, 0.75));
  const key = new THREE.DirectionalLight(0xffffff, 0.7);
  key.position.set(2, 3, 5);
  scene.add(key);

  // faint bounding box of the index space + axes so orientation is legible
  const box = new THREE.Box3(new THREE.Vector3(...pad(lo)), new THREE.Vector3(...pad(hi)));
  const boxHelper = new THREE.Box3Helper(box, 0x30363d);
  scene.add(boxHelper);
  const axes = new THREE.AxesHelper(span * 0.5);
  axes.position.set(...pad(lo));
  scene.add(axes);

  // tile-block boundaries: a faint wireframe box per block so the partition of
  // the index space into tile-sized chunks is visible under the wavefront. Cap
  // the box count so a pathological tile size (e.g. T=1 on a large domain) can't
  // spawn thousands of Box3Helpers — colour-by-block still conveys the partition.
  const totalBlocks = nTiles[0] * nTiles[1] * nTiles[2];
  if (tileMode && totalBlocks <= 512) {
    for (let K = 0; K < nTiles[2]; K++)
      for (let J = 0; J < nTiles[1]; J++)
        for (let I = 0; I < nTiles[0]; I++) {
          const t = [I, J, K];
          const blo = [0, 1, 2].map((d) => (lo[d] ?? 0) + t[d] * ts(d) - 0.5);
          const bhi = [0, 1, 2].map((d) => Math.min((hi[d] ?? 0) + 0.5, (lo[d] ?? 0) + (t[d] + 1) * ts(d) - 0.5));
          const b = new THREE.Box3(new THREE.Vector3(...blo), new THREE.Vector3(...bhi));
          scene.add(new THREE.Box3Helper(b, 0x586074));
        }
  }

  // ── one instanced sphere per activation — scales to 15^3 lattices (~10k
  //    activations for matmul's three recurrences) as a single draw call ─────
  const baseR = Math.min(0.3, Math.max(0.08, span * 0.05));
  const geo = new THREE.SphereGeometry(1, 8, 6);
  const inst = new THREE.InstancedMesh(
    geo, new THREE.MeshLambertMaterial({ transparent: true, opacity: 0.95 }), Math.max(1, acts.length));
  inst.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
  scene.add(inst);
  // fixed positions (small per-variable jitter so coincident variables, e.g.
  // matmul's a/b/c, don't perfectly overlap) + a per-activation base colour
  const jit = vars.length > 1 ? baseR * 0.55 : 0;
  const positions = acts.map((a) => [
    a.p[0] + (a.vi - (vars.length - 1) / 2) * jit, a.p[1], a.p[2],
  ]);
  const baseColors = acts.map((a) =>
    tileMode ? tileColor(tileId(a.p)) : new THREE.Color(PALETTE[a.vi % PALETTE.length]));
  const WHITE = new THREE.Color(0xffffff);
  // ── per-variable visibility (issue #142): with 3+ recurrences the coloured lattice
  //    is dense. A check-mark control (built in mountAll) toggles each variable's
  //    *activity wavefront*; when it's off, that variable's points fall back to plain
  //    GRAY index-space dots — as do the pending (not-yet-fired) points of the visible
  //    ones — so only the fired/firing wavefront of the checked variables carries colour.
  const varVisible = vars.map(() => true);
  const GRAY = new THREE.Color(0x6b7280);

  // ── τ-normal wavefront plane: a translucent quad whose normal is τ̂. Built once and
  //    oriented here; setFrame slides it ALONG τ̂ to the mean signature of the firing set,
  //    so it tracks the bright wavefront (β offsets and gaps included) and sweeps the cube.
  const tau3 = new THREE.Vector3();
  const tauHat3 = new THREE.Vector3(0, 0, 1);
  const ctrVec = new THREE.Vector3(...ctr);
  let tauLen = 1, tauHatDotCtr = 0, planeGrp = null;
  if (planeMode) {
    tau3.set(...tau3Arr);                                // every component validated finite above
    tauLen = tau3.length() || 1;
    tauHat3.copy(tau3).normalize();
    tauHatDotCtr = tauHat3.dot(ctrVec);
    const size = span * 1.8 + 2;                          // spans the whole cross-section
    const pg = new THREE.PlaneGeometry(size, size);
    const fill = new THREE.Mesh(pg, new THREE.MeshBasicMaterial({
      color: 0x4fd1ff, transparent: true, opacity: 0.12, side: THREE.DoubleSide, depthWrite: false }));
    const border = new THREE.LineSegments(new THREE.EdgesGeometry(pg),
      new THREE.LineBasicMaterial({ color: 0x4fd1ff, transparent: true, opacity: 0.5 }));
    planeGrp = new THREE.Group();
    planeGrp.add(fill, border);
    // orient the quad's +z normal onto τ̂; setFrame only translates it after this
    planeGrp.quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, 1), tauHat3);
    planeGrp.renderOrder = 2;
    scene.add(planeGrp);
  }

  // ── cross-tile dependency edges (issue #75): replay each variable's taps to
  //    find dependences that cross a tile boundary, and classify them — a
  //    |Δtile|∞ = 1 crossing is a HALO (nearest-neighbour, green); a longer jump
  //    is a COLLECTIVE (long-range, red). A uniform operator (all taps A = I)
  //    shows only green; an affine tap (a projection/permutation) lights up red.
  //    Edges are bucketed by the consumer's firing time so only the wavefront's
  //    traffic is drawn each frame. Tile mode only. ────────────────────────────
  const HALO_COLOR = new THREE.Color(0x35c759);
  const COLL_COLOR = new THREE.Color(0xff453a);
  const DEP_COLOR = new THREE.Color(0x9db4d0);    // neutral steel — plain dependency arrows
  const LEGAL_TIGHT = new THREE.Color(0x35c759);  // Δt = 1 — legal & latency-binding
  const LEGAL_SLACK = new THREE.Color(0x1f7a3f);  // Δt > 1 — legal, has slack
  const VIOL_COLOR = new THREE.Color(0xff453a);   // Δt ≤ 0 — dependence violated (illegal)
  const EDGE_CAP = 200000;
  let edgeBuckets = null, maxBucket = 0, haloTotal = 0, collTotal = 0, depTotal = 0, edgeCapped = false;
  let tightTotal = 0, slackTotal = 0, violTotal = 0;   // legality tallies over the whole schedule
  if (tileMode || edgesMode || legalityMode) {
    // index every activation by (variable name, point) so a tap resolves to a REAL
    // producer cell. A mapped coordinate can sit inside the global bounding box yet
    // be absent from the source variable's (e.g. triangular p ≤ q) domain — resolving
    // by name avoids drawing a phantom edge, and gives the producer's true position.
    const actAt = new Map();
    acts.forEach((a, i) => actAt.set(`${vars[a.vi].name}@${a.raw.join(',')}`, i));
    const applyMap = (A, b, p) =>
      A.map((row, r) => (b?.[r] ?? 0) + row.reduce((acc, m, c) => acc + m * (p[c] ?? 0), 0));
    edgeBuckets = Array.from({ length: frameCount }, () => []);
    let count = 0;
    for (let i = 0; i < acts.length && !edgeCapped; i++) {
      const a = acts[i];
      const consumerTile = tileMode ? tileOf(a.p) : null;
      for (const tap of vars[a.vi]?.taps ?? []) {
        if (!Array.isArray(tap.A)) continue;
        const s = applyMap(tap.A, tap.b, a.raw);
        const j = actAt.get(`${tap.source}@${s.join(',')}`);
        if (j === undefined) continue;              // boundary/input read or out-of-domain — no compute edge
        // Both overlays draw the SAME producer→consumer dependence; they differ only
        // in which edges survive and how they're coloured. `from` is the consumer,
        // `to` the producer — the per-frame refill gradient (edges mode) brightens
        // the consumer end so the direction of data flow reads.
        // cvi = the consumer's variable index, so the per-frame refill can drop edges
        // whose variable's wavefront the viewer has toggled off (checkbox control).
        const cvi = a.vi;
        if (tileMode) {
          const st = tileOf(acts[j].p);
          let lvl = 0;
          for (let d = 0; d < 3; d++) lvl = Math.max(lvl, Math.abs(consumerTile[d] - st[d]));
          if (lvl === 0) continue;                  // stays in the same tile — not cross-tile traffic
          edgeBuckets[a.t - tMin].push({
            from: positions[i], to: positions[j], color: lvl === 1 ? HALO_COLOR : COLL_COLOR, level: lvl, cvi });
          if (lvl === 1) haloTotal++; else collTotal++;
        } else if (legalityMode) {                  // colour by slack Δt = t(cons) − t(prod)
          const dt = a.t - acts[j].t;
          const kind = dt <= 0 ? 2 : (dt === 1 ? 1 : 0);   // 2 = violation, 1 = tight, 0 = slack
          const color = kind === 2 ? VIOL_COLOR : (kind === 1 ? LEGAL_TIGHT : LEGAL_SLACK);
          edgeBuckets[a.t - tMin].push({ from: positions[i], to: positions[j], color, level: kind, cvi });
          if (kind === 2) violTotal++; else if (kind === 1) tightTotal++; else slackTotal++;
        } else {                                    // edges mode: every compute dependence
          edgeBuckets[a.t - tMin].push({ from: positions[i], to: positions[j], color: DEP_COLOR, level: 0, cvi });
          depTotal++;
        }
        if (++count >= EDGE_CAP) { edgeCapped = true; break; }
      }
    }
    for (const bk of edgeBuckets) maxBucket = Math.max(maxBucket, bk.length);
  }
  // one LineSegments object, its draw range refilled per frame from the bucket
  let lineSeg = null;
  if ((tileMode || edgesMode || legalityMode) && maxBucket > 0) {
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(new Float32Array(maxBucket * 6), 3));
    g.setAttribute('color', new THREE.BufferAttribute(new Float32Array(maxBucket * 6), 3));
    lineSeg = new THREE.LineSegments(
      g, new THREE.LineBasicMaterial({ vertexColors: true, transparent: true, opacity: 0.85 }));
    scene.add(lineSeg);
  }
  let firingHalo = 0, firingColl = 0, firingDep = 0, firingTight = 0, firingSlack = 0, firingViol = 0;

  // ── per-frame state colouring ─────────────────────────────────────────────
  const dummy = new THREE.Object3D();
  const col = new THREE.Color();
  let frame = 0;
  function setFrame(f) {
    frame = Math.max(0, Math.min(frameCount - 1, Math.round(f)));
    const now = tMin + frame;
    let firing = 0, fired = 0, shown = 0, sigSum = 0;
    for (let i = 0; i < acts.length; i++) {
      const t = acts[i].t;
      const on = varVisible[acts[i].vi];
      let s;
      if (on) shown++;
      if (on && t === now) {               // the firing wavefront — bright, big
        s = baseR * 1.8; col.copy(baseColors[i]).lerp(WHITE, 0.6); firing++;
        // accumulate the signature σ = τ·p of the firing set to place the τ-plane
        if (planeGrp) { const p = acts[i].p; sigSum += tau3.x * p[0] + tau3.y * p[1] + tau3.z * p[2]; }
      } else if (on && t < now) {          // already fired — solid, small, dim
        s = baseR * 0.72; col.copy(baseColors[i]).multiplyScalar(0.7); fired++;
      } else {                             // pending, or a hidden variable — simple gray dot
        s = baseR * 0.5; col.copy(GRAY);
      }
      dummy.position.set(positions[i][0], positions[i][1], positions[i][2]);
      dummy.scale.setScalar(s);
      dummy.updateMatrix();
      inst.setMatrixAt(i, dummy.matrix);
      inst.setColorAt(i, col);
    }
    inst.instanceMatrix.needsUpdate = true;
    if (inst.instanceColor) inst.instanceColor.needsUpdate = true;

    // refill the overlay edges for the firing wavefront at this frame
    firingHalo = 0; firingColl = 0; firingDep = 0; firingTight = 0; firingSlack = 0; firingViol = 0;
    if (lineSeg) {
      const bucket = edgeBuckets[frame];
      const pos = lineSeg.geometry.attributes.position.array;
      const lc = lineSeg.geometry.attributes.color.array;
      let w = 0;                                   // packed edge count (hidden variables skipped)
      for (let n = 0; n < bucket.length; n++) {
        const e = bucket[n];
        if (!varVisible[e.cvi]) continue;          // this variable's wavefront is toggled off
        const c = e.color, o = w * 6;
        pos[o] = e.from[0]; pos[o + 1] = e.from[1]; pos[o + 2] = e.from[2];
        pos[o + 3] = e.to[0]; pos[o + 4] = e.to[1]; pos[o + 5] = e.to[2];
        // from = consumer, to = producer. In edges mode fade the producer end so the
        // segment reads as an arrow of data flow INTO the firing cell (cheaper than a
        // per-edge arrowhead cone); tile edges keep a flat colour at both ends.
        const dim = edgesMode ? 0.25 : 1;
        lc[o] = c.r; lc[o + 1] = c.g; lc[o + 2] = c.b;
        lc[o + 3] = c.r * dim; lc[o + 4] = c.g * dim; lc[o + 5] = c.b * dim;
        if (tileMode) { if (e.level === 1) firingHalo++; else firingColl++; }
        else if (legalityMode) { if (e.level === 2) firingViol++; else if (e.level === 1) firingTight++; else firingSlack++; }
        else firingDep++;
        w++;
      }
      lineSeg.geometry.setDrawRange(0, w * 2);
      lineSeg.geometry.attributes.position.needsUpdate = true;
      lineSeg.geometry.attributes.color.needsUpdate = true;
    }

    // slide the τ-plane to the firing set's mean signature: the plane {p : τ̂·p = along},
    // laterally centred on the cube. Hidden on frames where nothing fires (a schedule gap).
    if (planeGrp) {
      if (firing > 0) {
        planeGrp.visible = true;
        const along = sigSum / firing / tauLen;   // τ̂·p on the firing plane
        planeGrp.position.copy(ctrVec).addScaledVector(tauHat3, along - tauHatDotCtr);
      } else {
        planeGrp.visible = false;
      }
    }
    if (hud) {
      // tau is coerced to numbers so it can never carry markup; operator/kind are
      // escaped as they come straight from the fetched JSON.
      const tauTxt = Array.isArray(data.schedule?.tau)
        ? ` τ=[${data.schedule.tau.map((n) => Number(n)).join(',')}]` : '';
      hud.innerHTML =
        `<div>${esc(data.operator ?? 'schedule')} · ${esc(data.schedule?.kind ?? '')}` +
        tauTxt + `</div>` +
        `<div>step ${frame + 1} / ${frameCount}` +
        `&nbsp; latency ${data.latency ?? frameCount}</div>` +
        `<div>wavefront: <b>${firing}</b> firing (parallelism)</div>` +
        `<div>fired ${fired} / ${shown}</div>` +
        (planeMode ? `<div><span style="color:#4fd1ff">▬</span> wavefront plane ⟂ τ</div>` : '') +
        (tileMode ? `<div>tiles: ${nTiles[0]}×${nTiles[1]}×${nTiles[2]} (T=${tileSizes.join('×')})</div>` : '') +
        (lineSeg && tileMode ? `<div>cross-tile: <b style="color:#35c759">${firingHalo}</b> halo · ` +
          `<b style="color:#ff453a">${firingColl}</b> collective` +
          (edgeCapped ? ' <span class="sa-warn">(capped)</span>' : '') + `</div>` : '') +
        (edgesMode ? `<div>dependencies: <b style="color:#9db4d0">${firingDep}</b> firing` +
          (edgeCapped ? ' <span class="sa-warn">(capped)</span>' : '') + `</div>` : '') +
        (legalityMode ? `<div>schedule: ${
            violTotal > 0
              // a violation is conclusive even when capped — the schedule IS illegal
              ? `<b style="color:#ff453a">✗ ${violTotal} violation${violTotal === 1 ? '' : 's'}</b>`
              // but "✓ legal" requires having checked EVERY dependence: when the edge
              // builder hit its cap, an unchecked edge could still be a violation, so
              // withhold the certification rather than claim legality on partial data
              : (edgeCapped
                  ? '<b style="color:#ff9f0a">⚠ legality incomplete</b>'
                  : '<b style="color:#35c759">✓ legal</b>')
          }${edgeCapped ? ' <span class="sa-warn">(capped)</span>' : ''}</div>` +
          `<div>firing deps: <b style="color:#35c759">${firingTight}</b> tight · ${firingSlack} slack` +
          (firingViol ? ` · <b style="color:#ff453a">${firingViol} violated</b>` : '') + `</div>` : '');
    }
    options.onFrame?.(frame);
  }

  // ── camera fit + render loop ──────────────────────────────────────────────
  function fitView() {
    controls.target.set(...ctr);
    const d = span * 2.1 + 1;
    camera.position.set(ctr[0] + d, ctr[1] - d, ctr[2] + d * 0.8);
    camera.near = 0.01; camera.far = span * 60 + 100; camera.updateProjectionMatrix();
    controls.update();
  }
  function resize() {
    const w = canvas.clientWidth || 800, h = canvas.clientHeight || 460;
    renderer.setSize(w, h, false);
    camera.aspect = w / h; camera.updateProjectionMatrix();
  }

  // playback rate: an explicit `data-fps` (options.fps) wins; otherwise aim for a
  // ~7 s sweep, capped low enough that fine-grained schedules (e.g. QR's many
  // small steps) stay legible rather than flickering past.
  const fps = (Number.isFinite(options.fps) && options.fps > 0)
    ? options.fps
    : Math.max(1.5, Math.min(9, frameCount / 7));
  let playing = false, raf = 0, last = 0;
  function tick(ts) {
    raf = requestAnimationFrame(tick);
    if (playing && frameCount > 1 && ts - last > 1000 / fps) {
      last = ts;
      setFrame((frame + 1) % frameCount);
    }
    controls.update();
    renderer.render(scene, camera);
  }

  const ro = new ResizeObserver(resize);
  ro.observe(canvas);
  resize(); fitView(); setFrame(0);
  raf = requestAnimationFrame(tick);

  return {
    frameCount,
    tileMode,
    edgesMode,
    planeMode,
    legalityMode,
    varNames: vars.map((v) => String(v.name ?? '')),
    // toggle a variable's activity wavefront (checkbox control); off → gray index dots
    setVarVisible(vi, on) {
      if (vi >= 0 && vi < varVisible.length) { varVisible[vi] = !!on; setFrame(frame); }
    },
    setFrame,
    play() { playing = true; },
    pause() { playing = false; },
    fitView,
    dispose() {
      cancelAnimationFrame(raf); ro.disconnect(); controls.dispose();
      geo.dispose(); inst.material.dispose(); inst.dispose();
      if (lineSeg) { lineSeg.geometry.dispose(); lineSeg.material.dispose(); }
      if (planeGrp) planeGrp.traverse((o) => { o.geometry?.dispose?.(); o.material?.dispose?.(); });
      renderer.dispose();
    },
  };
}

// ── global mounter: turn every <div class="schedule-anim" data-src> into a
//    viewer with canvas + play/pause/scrub + a variable legend ───────────────
export async function mountAll(root = document) {
  reserveScheduleSlots(root);
  const nodes = root.querySelectorAll('.schedule-anim[data-src]:not([data-mounted])');
  for (const el of nodes) {
    el.setAttribute('data-mounted', '1');
    // Resolve data-src against the deploy base (import.meta.env.BASE_URL is
    // substituted at build), so "schedules/matmul-linear.json" works both at the
    // GitHub-Pages base (/domain_flow/) and in local dev (/).
    const raw = el.getAttribute('data-src');
    const base = (import.meta.env.BASE_URL || '/').replace(/\/$/, '');
    const src = /^https?:/.test(raw) ? raw : `${base}/${raw.replace(/^\//, '')}`;
    const height = el.getAttribute('data-height') || '460';
    const fpsAttr = Number(el.getAttribute('data-fps'));     // 0 / non-finite → adaptive default
    const fps = Number.isFinite(fpsAttr) && fpsAttr > 0 ? fpsAttr : 0;
    // data-tile="T" or "Tx,Ty,Tz" → colour activations by tile block (issue #70).
    // Keep every parsed entry (don't drop blanks) so a malformed tuple like
    // "5,,10" is rejected wholesale by the viewer rather than mis-aligned to [5,10].
    const tileRaw = el.getAttribute('data-tile');
    const tile = tileRaw ? tileRaw.split(',').map((s) => Number(s.trim())) : [];
    // data-edges → draw producer→consumer dependency arrows for the firing wavefront
    // (issue #142, Phase 3). A bare attribute (no value) is enough to turn it on;
    // ignored when data-tile is also present (tiles draw their own cross-tile edges).
    const edges = el.hasAttribute('data-edges');
    // data-legality → colour each dependence by its slack Δt (issue #142, Phase 3): red
    // violation, bright-green tight (Δt=1), dim-green slack. A bare attribute turns it on.
    const legality = el.hasAttribute('data-legality');
    // data-plane → draw the translucent τ-normal wavefront plane (issue #142, Phase 3).
    // A bare attribute turns it on; inert on free schedules (no τ). Composes with any mode.
    const plane = el.hasAttribute('data-plane');
    el.style.height = `${height}px`;

    const canvas = document.createElement('canvas');
    const hud = Object.assign(document.createElement('div'), { className: 'sa-hud' });
    const bar = Object.assign(document.createElement('div'), { className: 'sa-controls' });
    const play = Object.assign(document.createElement('button'), { type: 'button', textContent: '▶', title: 'play / pause' });
    const scrub = Object.assign(document.createElement('input'), { type: 'range', className: 'sa-scrub', min: '0', max: '0', value: '0', step: '1' });
    const fit = Object.assign(document.createElement('button'), { type: 'button', textContent: '⤢', title: 'reset view' });
    bar.append(play, scrub, fit);
    el.append(canvas, hud, bar);

    let data;
    try {
      const r = await fetch(src);
      if (!r.ok) throw new Error(`${r.status}`);
      data = await r.json();
    } catch (err) {
      hud.textContent = `failed to load ${src}: ${err.message}`;
      hud.classList.add('sa-warn');
      continue;
    }

    let playing = false;
    const viewer = createScheduleViewer({
      canvas, hud, data,
      options: { fps, tile, edges, plane, legality, onFrame: (i) => { if (playing) scrub.value = String(i); } },
    });
    scrub.max = String(Math.max(0, viewer.frameCount - 1));
    registerViewer(el, { setFrame: viewer.setFrame, frameCount: viewer.frameCount });

    // variable legend — suppressed in tile mode, where colour encodes the block
    // (one hue per tile) rather than the recurrence variable.
    if (viewer.tileMode) {
      const leg = Object.assign(document.createElement('div'), { className: 'sa-legend' });
      leg.innerHTML =
        `<span>colour = tile block · box = tile boundary · white = firing wavefront</span>` +
        `<span><i style="background:#35c759"></i>halo edge (nearest tile)</span>` +
        `<span><i style="background:#ff453a"></i>collective edge (long-range)</span>`;
      // the τ-plane composes with tile mode, so surface it in this legend branch too
      if (viewer.planeMode) {
        const s = document.createElement('span');
        s.innerHTML = `<i style="background:#4fd1ff"></i>wavefront plane ⟂ τ`;
        leg.append(s);
      }
      el.append(leg);
    } else if (Array.isArray(data.variables) && data.variables.length) {
      const leg = Object.assign(document.createElement('div'), { className: 'sa-legend' });
      const cols = ['#4fd1ff', '#ff9f0a', '#35c759', '#bf5af2', '#ffd60a', '#ff6482', '#64d2ff'];
      // each variable is a check-mark control: checked → its activity wavefront animates in
      // colour, unchecked → its points fall back to gray index-space dots. The label text is
      // a text node — v.name comes from the fetched JSON (data-src can be an arbitrary URL),
      // so it must never reach innerHTML.
      data.variables.forEach((v, i) => {
        const label = Object.assign(document.createElement('label'), { className: 'sa-var' });
        const cb = Object.assign(document.createElement('input'), { type: 'checkbox', checked: true });
        const sw = document.createElement('i');
        sw.style.background = cols[i % cols.length];
        cb.addEventListener('change', () => viewer.setVarVisible(i, cb.checked));
        label.append(cb, sw, document.createTextNode(String(v.name ?? '')));
        leg.append(label);
      });
      // dependency-arrow overlay legend (issue #142): the gradient reads producer → consumer
      if (viewer.edgesMode) {
        const s = document.createElement('span');
        s.innerHTML = `<i style="background:#9db4d0"></i>dependency (producer → consumer)`;
        leg.append(s);
      }
      // τ-normal wavefront plane legend (issue #142)
      if (viewer.planeMode) {
        const s = document.createElement('span');
        s.innerHTML = `<i style="background:#4fd1ff"></i>wavefront plane ⟂ τ`;
        leg.append(s);
      }
      // legality overlay legend (issue #142): slack Δt of each dependence
      if (viewer.legalityMode) {
        for (const [color, label] of [['#35c759', 'tight (Δt=1)'], ['#1f7a3f', 'slack (Δt>1)'], ['#ff453a', 'violation (Δt≤0)']]) {
          const s = document.createElement('span');
          const sw = document.createElement('i');
          sw.style.background = color;
          s.append(sw, label);
          leg.append(s);
        }
      }
      el.append(leg);
    }

    scrub.addEventListener('input', () => {
      playing = false; play.textContent = '▶'; viewer.pause(); viewer.setFrame(Number(scrub.value));
    });
    play.addEventListener('click', () => {
      playing = !playing; play.textContent = playing ? '⏸' : '▶';
      playing ? viewer.play() : viewer.pause();
    });
    fit.addEventListener('click', () => viewer.fitView());
  }
}

// ── side-by-side schedule comparison (issue #142, Phase 3): two viewers driven by ONE
//    shared clock so the latency↔parallelism trade reads directly — e.g. free vs linear.
//    Both advance at the same wall-clock step rate, so the shorter schedule finishes and
//    HOLDS its completed lattice while the longer one keeps sweeping: the latency gap is
//    the number of extra steps you watch it wait. Embed via
//    <div class="schedule-compare" data-src-a=".../op-free.json" data-src-b=".../op-linear.json">.
export async function mountCompareAll(root = document) {
  reserveScheduleSlots(root);
  const nodes = root.querySelectorAll('.schedule-compare[data-src-a][data-src-b]:not([data-mounted])');
  for (const el of nodes) {
    el.setAttribute('data-mounted', '1');
    const base = (import.meta.env.BASE_URL || '/').replace(/\/$/, '');
    const resolve = (raw) => /^https?:/.test(raw) ? raw : `${base}/${raw.replace(/^\//, '')}`;
    const height = el.getAttribute('data-height') || '420';
    const fpsAttr = Number(el.getAttribute('data-fps'));
    const fps0 = Number.isFinite(fpsAttr) && fpsAttr > 0 ? fpsAttr : 0;
    // overlays pass through to BOTH panes; each is guarded downstream (e.g. the τ-plane is
    // inert on a free schedule), so `data-plane` on a free-vs-linear compare lights up only
    // the linear pane — exactly the contrast the comparison is meant to show.
    const plane = el.hasAttribute('data-plane');
    const edges = el.hasAttribute('data-edges');
    const legality = el.hasAttribute('data-legality');
    const srcs = [el.getAttribute('data-src-a'), el.getAttribute('data-src-b')].map(resolve);

    const mk = (cls) => Object.assign(document.createElement('div'), { className: cls });
    const btn = (txt, title) => Object.assign(document.createElement('button'),
      { type: 'button', textContent: txt, title });

    const grid = mk('sc-grid');
    const panes = srcs.map(() => {
      const pane = mk('sc-pane'); pane.style.height = `${height}px`;
      const head = mk('sc-head');
      const canvas = document.createElement('canvas');
      const hud = mk('sa-hud');
      pane.append(head, canvas, hud);
      grid.append(pane);
      return { head, canvas, hud };
    });
    const bar = mk('sc-controls');
    const play = btn('▶', 'play / pause');
    const scrub = Object.assign(document.createElement('input'),
      { type: 'range', className: 'sa-scrub', min: '0', max: '0', value: '0', step: '1' });
    const fit = btn('⤢', 'reset view');
    bar.append(play, scrub, fit);
    el.append(grid, bar);

    // fetch both schedules (a failed pane shows an inline warning; the other still runs)
    const datas = [];
    for (let i = 0; i < 2; i++) {
      try {
        const r = await fetch(srcs[i]);
        if (!r.ok) throw new Error(`${r.status}`);
        datas[i] = await r.json();
      } catch (err) {
        panes[i].hud.textContent = `failed to load ${srcs[i]}: ${err.message}`;
        panes[i].hud.classList.add('sa-warn');
      }
    }

    const viewers = [];
    datas.forEach((data, i) => {
      if (!data) return;
      const v = createScheduleViewer({ canvas: panes[i].canvas, hud: panes[i].hud, data, options: { fps: fps0, plane, edges, legality } });
      v.pause();                                   // we drive frames from the shared clock
      viewers.push(v);
      const kind = data.schedule?.kind ?? '';
      const tau = Array.isArray(data.schedule?.tau) ? ` τ=[${data.schedule.tau.map((n) => Number(n)).join(',')}]` : '';
      panes[i].head.textContent = `${data.operator ?? 'schedule'} · ${kind}${tau} · latency ${data.latency ?? v.frameCount}`;
    });
    if (!viewers.length) continue;

    // shared legend: variable colours (both panes share the palette), plus overlay hints.
    // A swatch's label may be a variable name from the fetched JSON, so build it from a
    // text node (never innerHTML) — data-src can be an arbitrary URL and must not be able
    // to inject markup into the docs origin.
    const swatch = (color, label) => {
      const s = document.createElement('span');
      const i = document.createElement('i');
      i.style.background = color;
      s.append(i, String(label ?? ''));
      return s;
    };
    const first = datas.find(Boolean);
    if (Array.isArray(first?.variables) && first.variables.length) {
      const leg = mk('sc-legend');
      const cols = ['#4fd1ff', '#ff9f0a', '#35c759', '#bf5af2', '#ffd60a', '#ff6482', '#64d2ff'];
      // per-variable check-mark control — toggles the variable's wavefront in BOTH panes at
      // once (they share the recurrence), so a variable greys out on both sides together.
      first.variables.forEach((v, i) => {
        const label = Object.assign(document.createElement('label'), { className: 'sa-var' });
        const cb = Object.assign(document.createElement('input'), { type: 'checkbox', checked: true });
        const sw = document.createElement('i');
        sw.style.background = cols[i % cols.length];
        cb.addEventListener('change', () => { for (const v2 of viewers) v2.setVarVisible(i, cb.checked); });
        label.append(cb, sw, document.createTextNode(String(v.name ?? '')));
        leg.append(label);
      });
      if (plane) leg.append(swatch('#4fd1ff', 'wavefront plane ⟂ τ'));
      if (edges) leg.append(swatch('#9db4d0', 'dependency'));
      if (legality) {
        leg.append(swatch('#35c759', 'tight (Δt=1)'), swatch('#1f7a3f', 'slack (Δt>1)'), swatch('#ff453a', 'violation'));
      }
      el.append(leg);
    }

    // one clock for both: same step duration, so the shorter schedule clamps at its last
    // frame (all fired) while the longer keeps advancing — the visible latency difference.
    const maxFrames = Math.max(...viewers.map((v) => v.frameCount));
    scrub.max = String(Math.max(0, maxFrames - 1));
    const fps = fps0 || Math.max(1.5, Math.min(9, maxFrames / 7));
    let frame = 0, playing = false, raf = 0, last = 0, disposed = false;
    // Head.astro remounts on client-side navigation; when this element is removed the
    // shared loop tears itself and both viewers down, so detached canvases / WebGL
    // contexts / RAF loops don't leak across pages.
    const dispose = () => {
      if (disposed) return;
      disposed = true;
      cancelAnimationFrame(raf);
      for (const v of viewers) v.dispose();
    };
    const apply = (f) => { frame = f; for (const v of viewers) v.setFrame(f); scrub.value = String(f); };
    apply(0);
    registerViewer(el, { setFrame: apply, frameCount: maxFrames });
    function loop(ts) {
      if (!el.isConnected) { dispose(); return; }
      raf = requestAnimationFrame(loop);
      if (playing && maxFrames > 1 && ts - last > 1000 / fps) { last = ts; apply((frame + 1) % maxFrames); }
    }
    raf = requestAnimationFrame(loop);
    play.addEventListener('click', () => { playing = !playing; play.textContent = playing ? '⏸' : '▶'; });
    scrub.addEventListener('input', () => { playing = false; play.textContent = '▶'; apply(Number(scrub.value)); });
    fit.addEventListener('click', () => { for (const v of viewers) v.fitView(); });
  }
}

// ── offline capture hook (issue #142, Phase 3): publish each mounted viewer on a global
//    registry and tag its container with data-sv-index, so a headless driver (make-video.mjs)
//    can step it frame-by-frame — `window.__scheduleViewers[i].setFrame(f)` / `.frameCount` —
//    and read the canvas per frame for the offline PNG→video path. Harmless in the browser
//    (interactive controls still drive the same viewer); a no-op off-DOM. ──────────────────
//
// Slots are RESERVED synchronously in DOM order before any fetch: mountAll and mountCompareAll
// run concurrently and register only after their loads resolve, so without this the index a
// container ends up with would depend on network timing — and the capture's --index would
// drift between runs. Reserving up front pins data-sv-index to document order.
function reserveScheduleSlots(root) {
  if (typeof window === 'undefined') return;
  const reg = (window.__scheduleViewers ||= []);
  for (const el of root.querySelectorAll('.schedule-anim[data-src], .schedule-compare[data-src-a][data-src-b]')) {
    if (el.dataset.svIndex != null) continue;         // already reserved
    el.dataset.svIndex = String(reg.length);
    reg.push(null);                                   // placeholder; filled when the viewer mounts
  }
}
function registerViewer(el, handle) {
  if (typeof window === 'undefined') return;
  const reg = (window.__scheduleViewers ||= []);
  const i = Number(el.dataset.svIndex);
  if (Number.isInteger(i) && i >= 0) reg[i] = handle;  // fill the reserved slot
  else { el.dataset.svIndex = String(reg.length); reg.push(handle); }   // fallback: reserve wasn't run
}
