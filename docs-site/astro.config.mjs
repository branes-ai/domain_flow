import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import remarkMath from 'remark-math';
import rehypeKatex from 'rehype-katex';
import { BASE, REPO_URL } from './base.mjs';

export default defineConfig({
  site: 'https://branes-ai.github.io',
  base: BASE,
  markdown: {
    remarkPlugins: [remarkMath],
    rehypePlugins: [rehypeKatex],
  },
  integrations: [
    starlight({
      title: 'Domain Flow Architecture',
      description:
        'A header-only C++20 library for domain flow architecture parallelizing compilers',
      social: [
        {
          icon: 'github',
          label: 'GitHub',
          href: REPO_URL,
        },
      ],
      // No global editLink: src/content/docs/ is generated, so a global
      // baseUrl would 404. sync-content.mjs injects a per-page editUrl
      // pointing at each page's real source file instead.
      customCss: [
        'katex/dist/katex.min.css',
        './src/styles/custom.css',
      ],
      // Override <Head> to mount the SURE schedule animations (issue #64) — the
      // viewer + three.js load only on pages that embed a .schedule-anim div.
      components: {
        Head: './src/components/Head.astro',
      },
      sidebar: [
        {
          label: 'Getting Started',
          items: [{ autogenerate: { directory: 'getting-started' } }],
        },
        {
          label: 'Architecture',
          items: [{ autogenerate: { directory: 'architecture' } }],
        },
        {
          label: 'Theory',
          items: [{ autogenerate: { directory: 'theory' } }],
        },
        {
          label: 'SURE Simulator',
          items: [{ autogenerate: { directory: 'simulator' } }],
        },
        {
          label: 'SURE Algorithms',
          items: [
            {
              // Pedagogical order (each operator builds on the previous), not
              // alphabetical; pages live under content/docs/sure-algorithms/.
              label: 'BLAS L1',
              items: [
                { label: 'axpy — αx + y', slug: 'sure-algorithms/blas-l1/axpy' },
                { label: 'dot — xᵀy', slug: 'sure-algorithms/blas-l1/dot' },
                { label: 'nrm2 — ‖x‖₂', slug: 'sure-algorithms/blas-l1/nrm2' },
                { label: 'asum — Σ|xᵢ|', slug: 'sure-algorithms/blas-l1/asum' },
                { label: 'scal — αx', slug: 'sure-algorithms/blas-l1/scal' },
                { label: 'swap — x ↔ y', slug: 'sure-algorithms/blas-l1/swap' },
                { label: 'copy — y ← x', slug: 'sure-algorithms/blas-l1/copy' },
                { label: 'rot — Givens', slug: 'sure-algorithms/blas-l1/rot' },
                { label: 'iamax — argmax|xᵢ|', slug: 'sure-algorithms/blas-l1/iamax' },
              ],
            },
          ],
        },
        {
          label: 'Scaling & Distribution',
          items: [
            { label: 'The scale-out problem', slug: 'scaling' },
            { label: 'Tiling the index space', slug: 'scaling/tiling' },
            { label: 'Halo vs collective', slug: 'scaling/halo-vs-collective' },
            { label: 'Uniformization', slug: 'scaling/uniformization' },
            { label: 'The memory & communication hierarchy', slug: 'scaling/hierarchy' },
            { label: 'The Distributed Memory Machine', slug: 'scaling/dmm' },
            { label: 'Composition across the hierarchy', slug: 'scaling/composition' },
          ],
        },
        {
          label: 'Changelog',
          link: '/changelog/',
        },
      ],
    }),
  ],
});
