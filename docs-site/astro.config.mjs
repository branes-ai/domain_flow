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
          label: 'Theory',
          items: [{ autogenerate: { directory: 'theory' } }],
        },
        {
          label: 'Changelog',
          link: '/changelog/',
        },
      ],
    }),
  ],
});
