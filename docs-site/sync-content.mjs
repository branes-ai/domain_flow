#!/usr/bin/env node
/**
 * Syncs ALL documentation content from docs/ (and selected repo-root files)
 * into Starlight's src/content/docs/ tree.
 *
 * ARCHITECTURAL RULE: src/content/docs/ is 100% GENERATED.
 * ─────────────────────────────────────────────────────────
 * Every page — whether "hand-written" or transformed from repo docs —
 * originates in docs/.  Nothing is ever authored directly in
 * src/content/docs/.  The entire directory is .gitignored.
 *
 * Content categories:
 *   1. SITE pages  (docs/site/*.mdx) — copied verbatim (already have frontmatter)
 *   2. SYNCED docs (docs/**\/*.md)   — H1 extracted as title, frontmatter added,
 *                                      image paths & links rewritten
 *   3. ROOT files  (ARCHITECTURE.md, SETUP.md, CHANGELOG.md) — same transform
 *
 * Run automatically via `npm run build` / `npm run dev`.
 */

import { readFileSync, writeFileSync, mkdirSync, existsSync, cpSync, rmSync } from 'fs';
import { dirname, join, posix } from 'path';
import { BASE, REPO_URL } from './base.mjs';

const REPO = join(import.meta.dirname, '..');
const DOCS = join(REPO, 'docs');
const OUT  = join(import.meta.dirname, 'src', 'content', 'docs');

// ── Site pages (MDX with Starlight components) ────────────────────
// These are copied verbatim — they already contain frontmatter.
const SITE_FILES = {
  'site/index.mdx': 'index.mdx',
};

// ── Synced docs (source path relative to docs/ → dest relative to content/docs/) ──
const FILE_MAP = {
  // ── Architecture ────────────────────────────────────────────────
  'domain-flow-history.md':          'architecture/history.md',
  'ideation-of-dfg-technology.md':   'architecture/dfg-concepts.md',

  // ── SURE Simulator ──────────────────────────────────────────────
  'sure-simulator.md':               'simulator/index.md',

  // ── SURE Algorithms: BLAS Level 1 (executable operator derivations) ──
  // Order in the sidebar is set explicitly in astro.config.mjs (pedagogical,
  // not alphabetical).
  'SURE/blas-naming.md':             'sure-algorithms/blas-naming.md',
  'SURE/axpy.md':                    'sure-algorithms/blas-l1/axpy.md',
  'SURE/dot.md':                     'sure-algorithms/blas-l1/dot.md',
  'SURE/nrm2.md':                    'sure-algorithms/blas-l1/nrm2.md',
  'SURE/asum.md':                    'sure-algorithms/blas-l1/asum.md',
  'SURE/scal.md':                    'sure-algorithms/blas-l1/scal.md',
  'SURE/swap.md':                    'sure-algorithms/blas-l1/swap.md',
  'SURE/copy.md':                    'sure-algorithms/blas-l1/copy.md',
  'SURE/rot.md':                     'sure-algorithms/blas-l1/rot.md',
  'SURE/iamax.md':                   'sure-algorithms/blas-l1/iamax.md',

  // ── SURE Algorithms: BLAS Level 2 (lean reference: intro + SURE + animation;
  //    the derivation lives in Theory/matrix-vector-operators) ──
  'SURE/gemv.md':                    'sure-algorithms/blas-l2/gemv.md',
  'SURE/ger.md':                     'sure-algorithms/blas-l2/ger.md',
  'SURE/trmv.md':                    'sure-algorithms/blas-l2/trmv.md',
  'SURE/symv.md':                    'sure-algorithms/blas-l2/symv.md',
  'SURE/syr.md':                     'sure-algorithms/blas-l2/syr.md',
  'SURE/syr2.md':                    'sure-algorithms/blas-l2/syr2.md',

  // ── SURE Algorithms: BLAS Level 3 (lean reference) ──
  'SURE/gemm.md':                    'sure-algorithms/blas-l3/gemm.md',

  // ── Scaling & Distribution: mapping SUREs across tiles and SoCs ─────
  'scaling/index.md':                'scaling/index.md',
  'scaling/tiling.md':               'scaling/tiling.md',
  'scaling/halo-vs-collective.md':   'scaling/halo-vs-collective.md',
  'scaling/uniformization.md':       'scaling/uniformization.md',
  'scaling/hierarchy.md':            'scaling/hierarchy.md',
  'scaling/dmm.md':                  'scaling/dmm.md',
  'scaling/composition.md':          'scaling/composition.md',

  // ── Theory: SURE-construction methodology & derivations (explicit sidebar
  //    order in astro.config.mjs) ──
  'tensor_structure.md':             'theory/tensor-structure.md',
  'SURE/matmul.md':                  'theory/matmul.md',
  'alignment_formalism.md':          'theory/alignment-formalism.md',
  'anchoring.md':                    'theory/anchoring.md',
  'pipelining_schedules.md':         'theory/pipelining-schedules.md',
  'SURE/conv2d.md':                  'theory/conv2d.md',
  'SURE/QR_decomposition.md':        'theory/qr-decomposition.md',
  'matrix_vector_operators.md':      'theory/matrix-vector-operators.md',
};

// ── Root files (relative to repo root) ────────────────────────────
const ROOT_FILE_MAP = {
  'SETUP.md':        'getting-started/index.md',
  'ARCHITECTURE.md': 'architecture/index.md',
  'CHANGELOG.md':    'changelog.md',
};

// ── Link lookup ───────────────────────────────────────────────────

function buildLinkLookup() {
  const lookup = {};
  for (const [src, dest] of Object.entries(FILE_MAP)) {
    const slug = dest.replace(/\.md$/, '').replace(/\/index$/, '/');
    lookup[src] = `${BASE}/${slug.endsWith('/') ? slug : slug + '/'}`;
  }
  for (const [src, dest] of Object.entries(ROOT_FILE_MAP)) {
    const slug = dest.replace(/\.md$/, '').replace(/\/index$/, '/');
    lookup[`../${src}`] = `${BASE}/${slug.endsWith('/') ? slug : slug + '/'}`;
  }
  return lookup;
}

const LINK_LOOKUP = buildLinkLookup();

// ── Transforms ────────────────────────────────────────────────────

function rewriteLinks(content, srcRelative) {
  const srcDir = posix.dirname(srcRelative);
  return content.replace(/\]\(([^)]+\.md)\)/g, (match, target) => {
    if (target.startsWith('http://') || target.startsWith('https://')) return match;
    const resolved = posix.normalize(posix.join(srcDir, target));
    const url = LINK_LOOKUP[resolved];
    return url ? `](${url})` : match;
  });
}

function extractTitle(content) {
  const match = content.match(/^#\s+(.+)$/m);
  return match ? match[1].trim() : 'Untitled';
}

function stripFirstHeading(content) {
  return content.replace(/^#\s+.+\n*/m, '');
}

function rewriteImagePaths(content) {
  return content.replace(/\]\(images\//g, `](${BASE}/images/`);
}

function addFrontmatter(content, srcRelative) {
  const title = extractTitle(content);
  let body = stripFirstHeading(content);
  body = rewriteImagePaths(body);
  body = rewriteLinks(body, srcRelative);
  const safeTitle = title.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
  // Point "Edit this page" at the real committed source, not the generated tree.
  const repoPath = srcRelative.startsWith('../')
    ? srcRelative.slice('../'.length)   // repo-root file (SETUP.md, ...)
    : `docs/${srcRelative}`;            // docs/ file
  const editUrl = `${REPO_URL}/edit/main/${repoPath}`;
  return `---\ntitle: "${safeTitle}"\neditUrl: "${editUrl}"\n---\n\n${body}`;
}

// ── File operations ───────────────────────────────────────────────

function writeOut(destRelative, content) {
  const destPath = join(OUT, destRelative);
  mkdirSync(dirname(destPath), { recursive: true });
  writeFileSync(destPath, content);
}

let missingSources = 0;

function syncMarkdown(srcPath, srcRelative, destRelative) {
  if (!existsSync(srcPath)) {
    console.error(`  MISSING: ${srcPath}`);
    ++missingSources;
    return;
  }
  const content = readFileSync(srcPath, 'utf-8');
  writeOut(destRelative, addFrontmatter(content, srcRelative));
}

function copySitePage(srcPath, destRelative) {
  if (!existsSync(srcPath)) {
    console.error(`  MISSING: ${srcPath}`);
    ++missingSources;
    return;
  }
  // Site pages are authored with a %BASE% placeholder so the base path has a
  // single source of truth (base.mjs).
  const content = readFileSync(srcPath, 'utf-8').replaceAll('%BASE%', BASE);
  writeOut(destRelative, content);
}

// ── Main ──────────────────────────────────────────────────────────

// Clear stale Astro data store cache
const astroCache = join(import.meta.dirname, 'node_modules', '.astro');
if (existsSync(astroCache)) {
  rmSync(astroCache, { recursive: true });
}

// Wipe the entire output directory — it's 100% generated
if (existsSync(OUT)) {
  rmSync(OUT, { recursive: true });
}
mkdirSync(OUT, { recursive: true });

console.log('Syncing docs/ → docs-site/src/content/docs/ ...');

// 1. Copy site pages (MDX with components, already have frontmatter)
for (const [src, dest] of Object.entries(SITE_FILES)) {
  copySitePage(join(DOCS, src), dest);
  console.log(`  site: ${src} → ${dest}`);
}

// 2. Sync docs/ markdown files (add frontmatter, rewrite links)
for (const [src, dest] of Object.entries(FILE_MAP)) {
  syncMarkdown(join(DOCS, src), src, dest);
  console.log(`  sync: ${src} → ${dest}`);
}

// 3. Sync repo-root files
for (const [src, dest] of Object.entries(ROOT_FILE_MAP)) {
  syncMarkdown(join(REPO, src), `../${src}`, dest);
  console.log(`  root: ${src} → ${dest}`);
}

// 4. Copy images to public/ (served as static assets at /domain_flow/images/)
const PUB = join(import.meta.dirname, 'public');
const imgSrc = join(DOCS, 'images');
const imgDest = join(PUB, 'images');
if (existsSync(imgSrc)) {
  mkdirSync(imgDest, { recursive: true });
  cpSync(imgSrc, imgDest, { recursive: true });
  console.log('  Copied docs/images/ → public/images/');
}

// 5. Copy the committed SURE schedule-animation JSON (issue #64) to public/ so the
// docs build stays toolchain-free (regenerate via `npm run schedules`).
const schedSrc = join(DOCS, 'schedules');
const schedDest = join(PUB, 'schedules');
if (existsSync(schedSrc)) {
  mkdirSync(schedDest, { recursive: true });
  cpSync(schedSrc, schedDest, { recursive: true });
  console.log('  Copied docs/schedules/ → public/schedules/');
}

if (missingSources > 0) {
  console.error(`FAILED: ${missingSources} mapped source file(s) missing — fix the file map or restore the file(s).`);
  process.exit(1);
}

console.log('Done.');
