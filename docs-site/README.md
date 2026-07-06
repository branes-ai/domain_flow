# Domain Flow docs site

Astro + [Starlight](https://starlight.astro.build/) documentation site,
published to GitHub Pages at https://branes-ai.github.io/domain_flow/ by
`.github/workflows/docs.yml` on every push to `main` that touches docs.

## Authoring rule

**`src/content/docs/` is 100% generated — never author content there.**
All content lives in the repo's `docs/` tree (or `ARCHITECTURE.md` / `SETUP.md` /
`CHANGELOG.md` at the root) and is synced in by `sync-content.mjs`:

- `docs/site/*.mdx` — site pages (landing page) with Starlight components,
  copied verbatim
- entries in `FILE_MAP` / `ROOT_FILE_MAP` — plain repo markdown, transformed
  (H1 becomes the page title, links and image paths rewritten)

To publish a new document: add it under `docs/`, then add one line to
`FILE_MAP` in `sync-content.mjs`. The sidebar section is derived from the
destination directory (`getting-started/`, `architecture/`, `simulator/`,
`theory/`).

## Commands

```bash
npm install        # once
npm run dev        # sync content + local dev server
npm run build      # sync content + production build into dist/
npm run preview    # serve the production build locally
```

Math is rendered with KaTeX (`$...$` / `$$...$$` in markdown).
