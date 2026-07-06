// Single source of truth for the site's base path and repo URL.
// Imported by astro.config.mjs and sync-content.mjs; site MDX pages in
// docs/site/ reference the base via the %BASE% placeholder, which
// sync-content.mjs substitutes at sync time.
export const BASE = '/domain_flow';
export const REPO_URL = 'https://github.com/branes-ai/domain_flow';
