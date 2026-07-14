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

  // ── flatten activations, find the time range ──────────────────────────────
  const vars = data.variables ?? [];
  const acts = [];
  let tMin = Infinity, tMax = -Infinity;
  vars.forEach((v, vi) => {
    for (const pt of v.points) {
      acts.push({ p: pad(pt.p), t: pt.t, vi });
      tMin = Math.min(tMin, pt.t); tMax = Math.max(tMax, pt.t);
    }
  });
  if (!Number.isFinite(tMin)) { tMin = 0; tMax = 0; }
  const frameCount = tMax - tMin + 1;

  // ── renderer / scene / camera ─────────────────────────────────────────────
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
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

  // ── one sphere per activation (small lattices; instancing is a later step) ─
  // Each activation gets its OWN material: opacity/emissive are updated per point
  // per frame, so materials cannot be shared across a variable's points.
  const geo = new THREE.SphereGeometry(1, 14, 10);
  const baseR = Math.max(0.06, span * 0.05);
  const meshes = acts.map((a) => {
    const color = PALETTE[a.vi % PALETTE.length];
    const m = new THREE.Mesh(geo, new THREE.MeshPhongMaterial({ color, transparent: true, emissive: 0x000000 }));
    // jitter coincident variables (e.g. matmul a/b/c share coords) so all read
    const off = vars.length > 1 ? (a.vi - (vars.length - 1) / 2) * baseR * 0.9 : 0;
    m.position.set(a.p[0] + off, a.p[1], a.p[2]);
    m.userData.a = a;
    scene.add(m);
    return m;
  });

  // ── per-frame state colouring ─────────────────────────────────────────────
  let frame = 0;
  function setFrame(f) {
    frame = Math.max(0, Math.min(frameCount - 1, Math.round(f)));
    const now = tMin + frame;
    let firing = 0, fired = 0;
    meshes.forEach((m) => {
      const t = m.userData.a.t;
      const mat = m.material;
      if (t < now) {                       // already fired — solid, small
        m.visible = true; m.scale.setScalar(baseR * 0.7);
        mat.opacity = 0.9; mat.emissive.setHex(0x000000);
        fired++;
      } else if (t === now) {              // the firing wavefront — bright, big
        m.visible = true; m.scale.setScalar(baseR * 1.7);
        mat.opacity = 1; mat.emissive.setHex(mat.color.getHex());
        firing++;
      } else {                             // pending — faint
        m.visible = true; m.scale.setScalar(baseR * 0.5);
        mat.opacity = 0.12; mat.emissive.setHex(0x000000);
      }
    });
    if (hud) {
      hud.innerHTML =
        `<div>${data.operator ?? 'schedule'} · ${data.schedule?.kind ?? ''}` +
        (data.schedule?.tau ? ` τ=[${data.schedule.tau.join(',')}]` : '') + `</div>` +
        `<div>step ${frame + 1} / ${frameCount}` +
        `&nbsp; latency ${data.latency ?? frameCount}</div>` +
        `<div>wavefront: <b>${firing}</b> firing (parallelism)</div>` +
        `<div>fired ${fired} / ${acts.length}</div>`;
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

  let playing = false, raf = 0, last = 0;
  function tick(ts) {
    raf = requestAnimationFrame(tick);
    if (playing && frameCount > 1 && ts - last > 1000 / (options.fps ?? 1.5)) {
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
    setFrame,
    play() { playing = true; },
    pause() { playing = false; },
    fitView,
    dispose() { cancelAnimationFrame(raf); ro.disconnect(); controls.dispose(); renderer.dispose(); },
  };
}

// ── global mounter: turn every <div class="schedule-anim" data-src> into a
//    viewer with canvas + play/pause/scrub + a variable legend ───────────────
export async function mountAll(root = document) {
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
      options: { onFrame: (i) => { if (playing) scrub.value = String(i); } },
    });
    scrub.max = String(Math.max(0, viewer.frameCount - 1));

    // variable legend
    if (Array.isArray(data.variables) && data.variables.length) {
      const leg = Object.assign(document.createElement('div'), { className: 'sa-legend' });
      const cols = ['#4fd1ff', '#ff9f0a', '#35c759', '#bf5af2', '#ffd60a', '#ff6482', '#64d2ff'];
      data.variables.forEach((v, i) => {
        const s = document.createElement('span');
        s.innerHTML = `<i style="background:${cols[i % cols.length]}"></i>${v.name}`;
        leg.append(s);
      });
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
