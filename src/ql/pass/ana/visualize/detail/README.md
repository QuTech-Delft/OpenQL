Note: the actual visualizer sources in this `detail` folder and namespace are
essentially only compiled when CImg's dependencies are met, via the
`WITH_VISUALIZER` preprocessor definition. The sources *outside* of `detail` are
wrappers that are always compiled; they simply log an error when they are called
when the visualizer was not compiled. This way, a compilation strategy that
includes the visualizer doesn't fail because it can't visualize; after all, the
program would still compile just fine.

The rest of the codebase should not depend on `WITH_VISUALIZER`. Public header
files in particular *must* not depend on it, because someone using them to link
against OpenQL may well use different preprocessor options!
