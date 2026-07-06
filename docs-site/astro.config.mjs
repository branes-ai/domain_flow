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
          autogenerate: { directory: 'getting-started' },
        },
        {
          label: 'Architecture',
          autogenerate: { directory: 'architecture' },
        },
        {
          label: 'SURE Simulator',
          autogenerate: { directory: 'simulator' },
        },
        {
          label: 'Theory',
          autogenerate: { directory: 'theory' },
        },
        {
          label: 'Changelog',
          link: '/changelog/',
        },
      ],
    }),
  ],
});
