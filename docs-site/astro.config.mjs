import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import remarkMath from 'remark-math';
import rehypeKatex from 'rehype-katex';

export default defineConfig({
  site: 'https://branes-ai.github.io',
  base: '/domain_flow',
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
          href: 'https://github.com/branes-ai/domain_flow',
        },
      ],
      editLink: {
        baseUrl: 'https://github.com/branes-ai/domain_flow/edit/main/docs/',
      },
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
